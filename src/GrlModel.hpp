#include <vector>
#include "glm/glm.hpp"
#include "Program.h"

namespace grl {
	struct submodel
	{
		std::string name;
		std::vector<glm::vec3> posData;
		std::vector<glm::vec3> norData;
		std::vector<glm::vec2> texData;
		glm::mat4 O;
	};
}
class GrlModel {
private:
	std::vector<grl::submodel> subModels;
	unsigned int * vaoID;
	unsigned int * posBufID;
	unsigned int * norBufID;
	unsigned int * texBufID;

public:
	void loadSubModels(std::string filename);
	void init();
	void draw(
		const std::shared_ptr<Program> prog,
		glm::mat4 m = glm::mat4(1),
		glm::mat4 local_r = glm::mat4(1)
	);
	void drawSubModel(
		const std::shared_ptr<Program> prog,
		std::string subModelName,
		glm::mat4 m = glm::mat4(1),
		glm::mat4 local_r = glm::mat4(1)
	);
	void drawSubModel(
		const std::shared_ptr<Program> prog,
		unsigned int subModelIndex,
		glm::mat4 m = glm::mat4(1),
		glm::mat4 local_r = glm::mat4(1)
	);

	size_t getNumSubModels();
	std::string getNameOfSubModel(int index);

};