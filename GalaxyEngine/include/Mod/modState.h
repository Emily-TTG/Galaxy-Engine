#pragma once

// TODO: Fallback disable modding support instead of nuking the build.
#if defined(_MSC_VER) && !defined(__clang__)
# error "CL is not compatible with modding support"
#endif

struct GEModInitData {};
struct GEModInitReport {
	std::string name{ "<unset>" };
};

class GEMod {
public:
	static struct GEModInitReport Startup(const struct GEModInitData&);
	static bool CheckTerminal(const std::string&);
	static void Teardown();
};

class Mod {
	using HookFn = void();
	using CheckFn = bool(const std::string&);

	void* module{ nullptr };
	std::unordered_map<std::string, HookFn*> hooks{};
	struct GEModInitReport report;
	CheckFn* check_terminal;

public:
	explicit Mod(const std::string&);
	~Mod();

	// TODO: Way to provide a default return value instead of presuming default constructor is appropriate.
	template<typename Fn, typename... Args>
	std::pair<std::result_of_t<Fn&&(Args&&...)>, bool> HookAt(Fn* fn, Args&&... fwd) {
		auto name = SymName((HookFn*) fn);
		auto emplaced = hooks.try_emplace(name, nullptr);

		if(emplaced.second) {
			std::cout << "Populating hook `" << name << "` for mod `"  << report.name << "`" << std::endl;
			auto sym = GetSym(name);
			auto hook = (Fn*) sym;
			if(!hook) {
				std::cout << "`" << name << "` not found in mod `"  << report.name << "`" << std::endl;
				return { {}, false };
			}
			else {
				emplaced.first->second = sym;
			}
		}

		auto hook = (Fn*) emplaced.first->second;
		if(!hook) return { {}, false };

		const auto& ret = hook(std::forward<Args>(fwd)...);

		bool is_terminal = false;
		if(check_terminal) {
			auto demangled = Demangle(name);
			if(!demangled) {
				is_terminal = check_terminal(demangled.value());
			}
		}

		return { ret, is_terminal };
	}

	Mod(const Mod&) = delete;
	Mod& operator=(const Mod&) = delete;

	Mod(Mod&& other) noexcept;
	Mod& operator=(Mod&& other) noexcept;

private:
	HookFn* GetSym(const std::string&);
	std::optional<std::string> Demangle(const std::string&);
	std::string SymName(HookFn*);
};

class ModRegistry {
	static ModRegistry globalRegistry;

	std::vector<Mod> mods;

public:
	static auto& Get() { return globalRegistry; }

	Mod& Add(const std::string&);
};
