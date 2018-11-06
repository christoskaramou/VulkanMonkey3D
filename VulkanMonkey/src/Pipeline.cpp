#include "../include/Pipeline.h"
#include <iostream>

void Pipeline::destroy(vk::Device device)
{
	if (pipeinfo.layout) {
		device.destroyPipelineLayout(this->pipeinfo.layout);
		pipeinfo.layout = nullptr;
		std::cout << "Pipeline Layout destroyed\n";
	}

	if (pipeline) {
		device.destroyPipeline(pipeline);
		pipeline = nullptr;
		std::cout << "Pipeline destroyed\n";
	}
}