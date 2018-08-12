#include <vector>
#include "glm/glm.hpp"

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

};