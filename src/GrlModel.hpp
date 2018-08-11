#include <vector>
#include "glm/glm.hpp"

namespace grl {
	struct vertex {
		glm::vec3 pos, norm;
		glm::vec2 texCoord;
	};
	struct submodel
	{
		std::string name;
		std::vector<vertex*>* vertices;
		glm::mat4 O;
	};
}
class GrlModel {	
	std::vector<grl::submodel*> subModels;
	grl::vertex* loadVertex(std::vector<std::string> tokens);

public:
	void loadSubModels(std::string filename);

};