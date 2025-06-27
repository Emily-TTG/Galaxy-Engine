#pragma once

struct GEModInitData {};
struct GEModInitReport {
	std::string name{ "<unset>" };
};

class GEMod {
public:
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
				std::cout << "Populating hook `" << symbol << "` for mod `"  << report.name << "`" << std::endl;

				auto sym = GetSym(symbol);
				auto emplace = hooks.try_emplace(symbol, sym);
				if(!emplace.second) {
					std::cout << "Failed to emplace `" << symbol << "` in mod `"  << report.name << "`" << std::endl;
					return VoidOrDefault<Res>();
				}

				if(!sym) {
					std::cout << "`" << symbol << "` not found in mod `"  << report.name << "`" << std::endl;
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

	std::vector<Mod> mods;

public:
	static auto& Get() { return globalRegistry; }

	Mod& Add(const std::string &);
	auto& GetMods() { return mods; }
};

#define GE_HOOK(fn, ...) \
		do { \
			auto& hook_mods_ = ModRegistry::Get().GetMods(); \
			static const auto& hook_symbol_ = Mod::CurrentSym(); \
			for(auto& hook_mod_ : hook_mods_) { \
				hook_mod_.HookAt<decltype(&fn)>(hook_symbol_, ## __VA_ARGS__); \
            } \
		} while(0)
