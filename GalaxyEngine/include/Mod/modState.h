#pragma once

struct GEModInitData {};
// TODO: This needs a few extra pieces of info -- version, compat, description,
//  	 respath etc.
struct GEModInitReport {
	std::string name{ "<unset>" };
};

class GEMod {
public:
	// TODO: Allow user to enable/disable mods at runtime. Document that
	//		 teardown must remove any mod-specific data and may not represent
	//		 the end of program runtime.
	static struct GEModInitReport Startup(const struct GEModInitData&);
	static void Teardown();
};

class Mod {
public:
	using HookFn = void();

private:
	using CheckFn = bool(const std::string&);

	void* module{ nullptr };
	std::unordered_map<std::string, HookFn*> hooks{};
	struct GEModInitReport report;
	std::shared_mutex lock;

public:
	explicit Mod(const std::string&);
	~Mod();

	bool Valid() { return !!module; }

	// TODO: Way to provide a default return value instead of presuming default constructor is appropriate.
	template<typename Fn, typename... Args>
	auto HookAt(const std::string& symbol, Args&&... fwd) {
		using Res = std::result_of_t<Fn&&(Args&&...)>;
		using SymFn = ResultOf<Fn>::Type(Args...);

		auto it = hooks.find(symbol);

		if(it == hooks.end()) {
			std::unique_lock write{ lock };

			// Try again in case someone else beat us to it.
			it = hooks.find(symbol);
			if(it == hooks.end()) {
				//std::cout << "Populating hook `" << symbol << "` for mod `"  << report.name << "`" << std::endl;

				auto sym = GetSym(symbol);
				auto emplace = hooks.try_emplace(symbol, sym);
				if(!emplace.second) {
					std::cout << "Failed to emplace `" << symbol << "` in mod `"  << report.name << "`" << std::endl;
					return VoidOrDefault<Res>();
				}

				if(!sym) {
					//std::cout << "`" << symbol << "` not found in mod `"  << report.name << "`" << std::endl;
					return VoidOrDefault<Res>();
				}
				else {
					it = emplace.first;
				}
			}
		}

		std::shared_lock read{ lock };

		auto hook = (SymFn*) it->second;
		if(!hook) return VoidOrDefault<Res>();

		return hook(std::forward<Args>(fwd)...);
	}

	static std::string SymName(HookFn*);
	static std::string CurrentSym();

	Mod(const Mod&) = delete;
	Mod& operator=(const Mod&) = delete;

	Mod(Mod&& other) noexcept;
	Mod& operator=(Mod&& other) noexcept;

private:
	template<typename T>
	struct ResultOf;

	template<typename R, typename... Args>
	struct ResultOf<R (*)(Args...)> {
		using Type = R;
	};

	template<typename C, typename R, typename... Args>
	struct ResultOf<R (C::*)(Args...)> {
		using Type = R;
	};

	template<typename Fn, typename... Args>
	inline auto CallSym(const std::string& symbol, Args&&... fwd) {
		return ((Fn*) GetSym(symbol))(std::forward<Args>(fwd)...);
	}

	HookFn* GetSym(const std::string&);

	template<typename T>
	inline T VoidOrDefault() { return {}; }
};

template<>
inline void Mod::VoidOrDefault() {}

class ModRegistry {
	static ModRegistry globalRegistry;
	static thread_local bool terminate;

	std::vector<Mod> mods;

public:
	static void Terminate() { terminate = true; }
	static bool GetTerminate() { return terminate; }
	static void ClearTerminate() { terminate = false; }

	static auto& Get() { return globalRegistry; }

	void Add(const std::string& path) {
		Mod mod{ path };
		if(mod.Valid()) mods.emplace_back(std::move(mod));
	}

	auto& GetMods() { return mods; }
};

// NOTE: This is a macro rather than a function for the following reasons:
//		 a) the persistent `hook_symbol_` static needs to be held at callsite.
//		 b) post hooks use a RAII'd object to hook on all frame exits.
//		 c) hooks in future will be able to force early return from the vanilla proc.
#define GE_HOOK(fn, ...) \
		do { \
			auto& hook_mods_ = ModRegistry::Get().GetMods(); \
			static const auto& hook_symbol_ = Mod::CurrentSym(); \
			for(auto& hook_mod_ : hook_mods_) { \
				auto&& hook_return_ = hook_mod_.HookAt<decltype(&fn)>(hook_symbol_, ## __VA_ARGS__); \
            	if(ModRegistry::GetTerminate()) { \
					ModRegistry::ClearTerminate(); \
					return hook_return_; \
				} \
            } \
		} while(0)

#define GE_HOOK_V(fn, ...) \
		do { \
			auto& hook_mods_ = ModRegistry::Get().GetMods(); \
			static const auto& hook_symbol_ = Mod::CurrentSym(); \
			for(auto& hook_mod_ : hook_mods_) { \
				hook_mod_.HookAt<decltype(&fn)>(hook_symbol_, ## __VA_ARGS__); \
            	if(ModRegistry::GetTerminate()) { \
					ModRegistry::ClearTerminate(); \
					return; \
				} \
            } \
		} while(0)
