#include <Mod/modState.h>

#ifdef _WIN32
#else
# include <dlfcn.h>
#endif

#ifndef _MSC_VER
# include <cxxabi.h>
#endif

ModRegistry ModRegistry::globalRegistry;

// TODO: If/when the GE core itself is split out into a module -- this should
//  	 Be populated.
struct GEModInitReport GEMod::Startup(const struct GEModInitData&) { return {}; }
bool GEMod::CheckTerminal(const std::string&) { return {}; }
void GEMod::Teardown() {}

Mod::Mod(const std::string& path) {
#ifdef _WIN32
#else
	// TODO: This is a glibc feature -- feature test and fallback.
	module = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
	if(!module) {
		std::cerr << "Failed to load mod `" << path << "`: " << dlerror() << std::endl;
	}

	auto& info = typeid(GEMod::CheckTerminal);
	auto name = info.name();
	check_terminal = (CheckFn*) GetSym(name);
#endif

	struct GEModInitData init;
	auto eval = HookAt(&GEMod::Startup, init);
	std::cout << "Mod `" << eval.first.name << "` loaded" << std::endl;
}

Mod::~Mod() {
	if(module) dlclose(module);
}

Mod::Mod(Mod&& other) noexcept {
	module = nullptr;
	hooks = std::move(other.hooks);
	report = std::move(other.report);
	check_terminal = other.check_terminal;
}

Mod& Mod::operator=(Mod&& other) noexcept {
	module = nullptr;
	hooks = std::move(other.hooks);
	report = std::move(other.report);
	check_terminal = other.check_terminal;
	return *this;
}

Mod::HookFn* Mod::GetSym(const std::string& sym) {
#ifdef _WIN32
#else
	return (HookFn*) dlsym(module, sym.c_str());
#endif
}

std::optional<std::string> Mod::Demangle(const std::string& mangled) {
#ifdef _WIN32
#else
	int result;
	char* demangled = abi::__cxa_demangle(mangled.c_str(), nullptr, nullptr, &result);
	if(!result) {
		auto ret = std::string(demangled);
		free(demangled);
		return ret;
	}

	return std::nullopt;
#endif
}

std::string Mod::SymName(HookFn* fn) {
#ifdef _WIN32
#else
	// TODO: This is a glibc feature -- feature test and fallback.
	Dl_info info;
	int result = dladdr((void*) fn, &info);
	if(!result) return "<unk>";

	return info.dli_sname;
#endif
}

Mod& ModRegistry::Add(const std::string& path) {
	return mods.emplace_back(path);
}
