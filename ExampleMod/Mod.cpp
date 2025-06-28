#include "Physics/physics.h"

#include "Mod/modState.h"

struct GEModInitReport GEMod::Startup(const struct GEModInitData&) {
	return { "testmod" };
}

void GEMod::Teardown() {

}

void saveConfig() {
	if (!std::filesystem::exists("Config")) {
		std::filesystem::create_directory("Config");
	}

	std::ofstream file("Config/myModConfig.txt");

	YAML::Emitter out(file);
	out << YAML::BeginMap;
	out << YAML::Key << "Moderation" << YAML::Value << 3;
	out << YAML::Key << "Modification" << YAML::Value << "aaa";
	out << YAML::EndMap;
}

void loadConfig() {
	YAML::Node config = YAML::LoadFile("Config/myModConfig.txt");

	if (config["Modification"])
		printf("There be mods! %s\n", config["Modification"].as<std::string>());
}

glm::vec2 Physics::calculateForceFromGrid(const Quadtree& grid, std::vector<ParticlePhysics>& pParticles, UpdateVariables& myVar, ParticlePhysics& pParticle) {
	return { 0, 0 };
}
