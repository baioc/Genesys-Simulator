/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   PluginManager.cpp
 * Author: rafael.luiz.cancian
 *
 * Created on 30 de Maio de 2019, 17:49
 */

#include "PluginManager.h"
#include "Simulator.h"
#include "../TraitsKernel.h"
#include "Attribute.h"
#include "Counter.h"
#include "EntityType.h"
#include "StatisticsCollector.h"
#include "Model.h"

//using namespace GenesysKernel;

PluginManager::PluginManager(Simulator* simulator) {
	_simulator = simulator;
	this->_pluginConnector = new TraitsKernel<PluginConnector_if>::Implementation();
	this->_insertDefaultKernelElements();
}

std::string PluginManager::show() {
	std::string message = "Plugins=[";
	for (Plugin* plugin : *_plugins->list()) {
		message += +"{" + plugin->show() + "}, ";
	}
	message = message.substr(0, message.length() - 2);
	message += "]";
	return message;
}

void PluginManager::_insertDefaultKernelElements() {
	StaticGetPluginInformation GetInfo;
	GetInfo = &EntityType::GetPluginInformation;
	_plugins->insert(new Plugin(GetInfo));
	GetInfo = &Attribute::GetPluginInformation;
	_plugins->insert(new Plugin(GetInfo));
	GetInfo = &Counter::GetPluginInformation;
	_plugins->insert(new Plugin(GetInfo));
	GetInfo = &StatisticsCollector::GetPluginInformation;
	_plugins->insert(new Plugin(GetInfo));
}

bool PluginManager::completePluginsFieldsAndTemplates() {
	return _simulator->_completePluginsFieldsAndTemplate();
}

//bool PluginManager::check(Plugin* plugin){
//    return true;
//}

bool PluginManager::_insert(Plugin * plugin) {
	PluginInformation *plugInfo = plugin->getPluginInfo();
	if (plugin->isIsValidPlugin() && plugInfo != nullptr) {
		std::string msg = "Inserting ";
		if (plugInfo->isComponent())
			msg += "component";
		else
			msg += "modeldatum";
		msg += " plugin \"" + plugin->getPluginInfo()->getPluginTypename() + "\"";
		_simulator->getTracer()->trace(Util::TraceLevel::L8_detailed, msg);
		// insert all dependencies before to insert this plugin
		bool allDependenciesInserted = true;
		if (plugInfo->getDynamicLibFilenameDependencies()->size() > 0) {
			Util::IncIndent();
			{
				_simulator->getTracer()->trace(Util::TraceLevel::L8_detailed, "Inserting dependencies...");
				Util::IncIndent();
				{
					for (std::string str : *plugInfo->getDynamicLibFilenameDependencies()) {
						allDependenciesInserted &= (this->insert(str) != nullptr);
					}
				}
				Util::DecIndent();
			}
			Util::DecIndent();
		}
		if (!allDependenciesInserted) {
			_simulator->getTracer()->trace(Util::TraceLevel::L3_errorRecover, "Plugin dependencies could not be inserted; therefore, plugin will not be inserted");
			return false;
		}
		if (this->find(plugInfo->getPluginTypename()) != nullptr) { // plugin alread exists
			Util::IncIndent();
			_simulator->getTracer()->trace(Util::TraceLevel::L8_detailed, "Plugin alread exists and was not inserted again");
			Util::DecIndent();
			return false;
		}
		_plugins->insert(plugin);
		Util::IncIndent();
		this->_simulator->getTracer()->trace(Util::TraceLevel::L2_results, "Plugin successfully inserted");
		Util::DecIndent();
		return true;
	} else {
		Util::IncIndent();
		this->_simulator->getTracer()->trace(Util::TraceLevel::L2_results, "Plugin could not be inserted");
		plugin->~Plugin(); // destroy the invalid plugin
		Util::DecIndent();
		return false;
	}
}

bool PluginManager::check(std::string dynamicLibraryFilename) {
	Plugin* plugin;
	try {
		plugin = _pluginConnector->check(dynamicLibraryFilename);
	} catch (...) {
		return false;
	}
	return (plugin != nullptr);
}

Plugin * PluginManager::insert(std::string dynamicLibraryFilename) {
	Plugin* plugin;
	try {
		plugin = _pluginConnector->connect(dynamicLibraryFilename);
		if (plugin != nullptr)
			_insert(plugin);
		else {
			_simulator->getTracer()->trace(Util::TraceLevel::L3_errorRecover, "Plugin from file \"" + dynamicLibraryFilename + "\" could not be loaded.");
		}
	} catch (...) {

		return nullptr;
	}
	return plugin;
}

bool PluginManager::remove(std::string dynamicLibraryFilename) {

	Plugin* pi = this->find(dynamicLibraryFilename);
	return remove(pi);
}

bool PluginManager::remove(Plugin * plugin) {
	if (plugin != nullptr) {
		_plugins->remove(plugin);
		try {
			_pluginConnector->disconnect(plugin);
		} catch (...) {
			return false;
		}
		_simulator->getTracer()->trace(Util::TraceLevel::L2_results, "Plugin successfully removed");
		return true;
	}
	_simulator->getTracer()->trace(Util::TraceLevel::L2_results, "Plugin could not be removed");
	return false;
}

Plugin * PluginManager::find(std::string pluginTypeName) {
	for (std::list<Plugin*>::iterator it = this->_plugins->list()->begin(); it != _plugins->list()->end(); it++) {
		if ((*it)->getPluginInfo()->getPluginTypename() == pluginTypeName) {

			return (*it);
		}
	}
	return nullptr;
}

Plugin * PluginManager::front() {
	return this->_plugins->front();
}

Plugin * PluginManager::next() {
	return _plugins->next();
}

Plugin * PluginManager::last() {
	return this->_plugins->last();
}

unsigned int PluginManager::size() {
	return _plugins->size();
}

Plugin * PluginManager::getAtRank(unsigned int rank) {
	return _plugins->getAtRank(rank);
}

