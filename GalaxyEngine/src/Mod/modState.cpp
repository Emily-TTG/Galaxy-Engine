#include <Mod/modState.h>

#ifdef _WIN32
#else
# include <dlfcn.h>
# include <execinfo.h>
#endif

ModRegistry ModRegistry::globalRegistry;

struct GEModInitReport GEMod::Startup(const struct GEModInitData& data) {
	return {};
}

void GEMod::Teardown() {}

Mod::Mod(const std::string& path) {
#ifdef _WIN32
#else
	// TODO: This is a glibc feature -- feature test and fallback.
	module = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
	if(!module) {
		std::cerr << "Failed to load mod `" << path << "`: " << dlerror() << std::endl;
	}
#endif

	report = CallSym<decltype(GEMod::Startup)>(SymName((HookFn*) &GEMod::Startup), GEModInitData{});
	std::cout << "Mod `" << report.name << "` loaded" << std::endl;
}

Mod::~Mod() {
	if(module) dlclose(module);
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

std::string Mod::CurrentSym() {
#ifdef _WIN32
#else
	void* array[2];
	backtrace(array, std::size(array));

	return SymName((HookFn*) array[1]);
#endif
}

Mod::Mod(Mod&& other) noexcept {
	module = nullptr;
	hooks = std::move(other.hooks);
	report = std::move(other.report);
}

Mod& Mod::operator=(Mod&& other) noexcept {
	module = nullptr;
	hooks = std::move(other.hooks);
	report = std::move(other.report);
	return *this;
}

Mod::HookFn* Mod::GetSym(const std::string& sym) {
#ifdef _WIN32
#else
	return (HookFn*) dlsym(module, sym.c_str());
#endif
}

Mod& ModRegistry::Add(const std::string& path) {
	return mods.emplace_back(path);
}
