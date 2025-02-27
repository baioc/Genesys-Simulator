/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Smart_Process.cpp
 * Author: rlcancian
 * 
 * Created on 3 de Setembro de 2019, 18:34
 */

#include "Smart_Process.h"

// you have to included need libs

// GEnSyS Simulator
#include "../../kernel/simulator/Simulator.h"

// Model Components
#include "../../plugins/components/Create.h"
#include "../../plugins/components/Process.h"
#include "../../plugins/components/Dispose.h"

Smart_Process::Smart_Process() {
}

/**
 * This is the main function of the application. 
 * It instanciates the simulator, builds a simulation model and then simulate that model.
 */
int Smart_Process::main(int argc, char** argv) {
	Simulator* genesys = new Simulator();
	this->insertFakePluginsByHand(genesys);
	this->setDefaultTraceHandlers(genesys->getTracer());
	genesys->getTracer()->setTraceLevel(Util::TraceLevel::L9_mostDetailed);
	// crete model
	Model* model = genesys->getModels()->newModel();
	PluginManager* plugins = genesys->getPlugins();
	Create *create = plugins->newInstance<Create>(model);
	Process* process = plugins->newInstance<Process>(model);
	process->getSeizeRequests()->insert(new SeizableItem(plugins->newInstance<Resource>(model)));
	process->setQueueableItem(new QueueableItem(plugins->newInstance<Queue>(model)));
	process->setDelayExpression("unif(0.7,1.5)");
	Dispose* dispose = plugins->newInstance<Dispose>(model);
	// connect model components to create a "workflow"
	create->getConnections()->insert(process);
	process->getConnections()->insert(dispose);
	// set options, save and simulate step-by-step (but no user interaction required)
	model->getSimulation()->setReplicationLength(10);
	model->save("./models/Smart_Process.gen");
	do {
		model->getSimulation()->step();
	} while (model->getSimulation()->isPaused());
	genesys->~Simulator();
	return 0;
};

