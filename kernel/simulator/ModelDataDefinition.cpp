/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ModelDataDefinition.cpp
 * Author: rafael.luiz.cancian
 * 
 * Created on 21 de Junho de 2018, 19:40
 */

//#include <typeinfo>
#include "ModelDataDefinition.h"
#include <iostream>   
#include <cassert>
#include "Model.h"
#include "../TraitsKernel.h"

//using namespace GenesysKernel;

ModelDataDefinition::ModelDataDefinition(Model* model, std::string thistypename, std::string name, bool insertIntoModel) {
	_id = Util::GenerateNewId(); //GenerateNewIdOfType(thistypename);
	_typename = thistypename;
	_parentModel = model;
	_reportStatistics = TraitsKernel<ModelDataDefinition>::reportStatistics; // @TODO: shoould be a parameter before insertIntoModel
	if (name == "")
		_name = thistypename + "_" + std::to_string(Util::GenerateNewIdOfType(thistypename));
	else if (name.substr(name.length() - 1, 1) == "%")
		_name = name.substr(0, name.length() - 1) + std::to_string(Util::GenerateNewIdOfType(thistypename));
	else
		_name = name;
	_hasChanged = false;
	if (insertIntoModel) {
		model->insert(this);
	}
}

bool ModelDataDefinition::hasChanged() const {
	return _hasChanged;
}

//ModelDataDefinition::ModelDataDefinition(const ModelDataDefinition &orig) {
//this->_parentModel = orig->_parentModel;
//this->_name = "copy_of_" + orig->_name;
//this->_typename = orig->_typename;
//}

ModelDataDefinition::~ModelDataDefinition() {
	////_parentModel->getTracer()->trace(Util::TraceLevel::L9_mostDetailed, "Removing Element \"" + this->_name + "\" from the model");
	_removeInternalDatas();
	_parentModel->getDataManager()->remove(this);
}

void ModelDataDefinition::_removeInternalDatas() {
	Util::IncIndent();
	{
		for (std::map<std::string, ModelDataDefinition*>::iterator it = _internalData->begin(); it != _internalData->end(); it++) {
			this->_parentModel->getDataManager()->remove((*it).second);
			(*it).second->~ModelDataDefinition();
		}
		_internalData->clear();
	}
	Util::DecIndent();

}

void ModelDataDefinition::_removeInternalData(std::string key) {
	Util::IncIndent();
	{
		//for (std::map<std::string, ModelDataDefinition*>::iterator it = _internelElements->begin(); it != _internelElements->end(); it++) {
		std::map<std::string, ModelDataDefinition*>::iterator it = _internalData->begin();
		while (it != _internalData->end()) {
			if ((*it).first == key) {
				this->_parentModel->getDataManager()->remove((*it).second);
				(*it).second->~ModelDataDefinition();
				_internalData->erase(it);
				it = _internalData->begin();
			}
		}
	}
	Util::DecIndent();
}

bool ModelDataDefinition::_getSaveDefaultsOption() {
	return _parentModel->getPersistence()->getOption(ModelPersistence_if::Options::SAVEDEFAULTS);
}

bool ModelDataDefinition::_loadInstance(std::map<std::string, std::string>* fields) {
	bool res = true;
	std::map<std::string, std::string>::iterator it;
	it = fields->find("id");
	it != fields->end() ? this->_id = std::stoi((*it).second) : res = false;
	it = fields->find("name");
	it != fields->end() ? this->_name = (*it).second : this->_name = "";
	//it != fields->end() ? this->_name = (*it).second : res = false;
	it = fields->find("reportStatistics");
	it != fields->end() ? this->_reportStatistics = std::stoi((*it).second) : this->_reportStatistics = TraitsKernel<ModelDataDefinition>::reportStatistics;
	return res;
}

std::map<std::string, std::string>* ModelDataDefinition::_saveInstance(bool saveDefaultValues) {
	std::map<std::string, std::string>* fields = new std::map<std::string, std::string>();
	SaveField(fields, "typename", _typename);
	SaveField(fields, "id", this->_id);
	SaveField(fields, "name", _name);
	SaveField(fields, "reportStatistics", _reportStatistics, TraitsKernel<ModelDataDefinition>::reportStatistics, saveDefaultValues);
	return fields;
}

bool ModelDataDefinition::_check(std::string* errorMessage) {
	*errorMessage += "";
	return true; // if there is no ovveride, return true
}

ParserChangesInformation* ModelDataDefinition::_getParserChangesInformation() {
	return new ParserChangesInformation(); // if there is no override, return no changes
}

void ModelDataDefinition::_initBetweenReplications() {
	for (std::pair<std::string, ModelDataDefinition*> pair : *_internalData) {
		pair.second->_initBetweenReplications();
	}
}

/*
std::list<std::map<std::string,std::string>*>* ModelDataDefinition::_saveInstance(std::string type) {
	std::list<std::map<std::string,std::string>*>* fields = ModelDataDefinition::_saveInstance(saveDefaultValues);
	fields->push_back(type);
	return fields;
}
 */

std::string ModelDataDefinition::show() {
	std::string internel = "";
	if (_internalData->size() > 0) {
		internel = ", internel=[";
		for (std::map<std::string, ModelDataDefinition*>::iterator it = _internalData->begin(); it != _internalData->end(); it++) {
			internel += (*it).second->getName() + ",";
		}
		internel = internel.substr(0, internel.length() - 1) + "]";
	}
	return "id=" + std::to_string(_id) + ",name=\"" + _name + "\"" + internel;
}

std::list<std::string>* ModelDataDefinition::getInternalDatasKeys() const {
	std::list<std::string>* result = new std::list<std::string>();
	return result;
}

Util::identification ModelDataDefinition::getId() const {
	return _id;
}

void ModelDataDefinition::setName(std::string name) {
	// rename every "stuff" related to this modeldatum (controls, responses and internelElements)
	if (name != _name) {
		std::string stuffName;
		unsigned int pos;
		for (std::pair<std::string, ModelDataDefinition*> child : *_internalData) {
			stuffName = child.second->getName();
			pos = stuffName.find(getName(), 0);
			if (pos < stuffName.length()) {// != std::string::npos) {
				stuffName = stuffName.replace(pos, pos + getName().length(), name);
				child.second->setName(stuffName);
			}
		}

		for (SimulationControl* control : *_parentModel->getControls()->list()) {
			stuffName = control->getName();
			pos = stuffName.find(getName(), 0);
			if (pos < stuffName.length()) { // != std::string::npos) {
				stuffName = stuffName.replace(pos, pos + getName().length(), name);
				control->setName(stuffName);
			}
		}

		for (SimulationResponse* response : *_parentModel->getResponses()->list()) {
			stuffName = response->getName();
			pos = stuffName.find(getName(), 0);
			if (pos < stuffName.length()) {// != std::string::npos) {
				stuffName = stuffName.replace(pos, pos + getName().length(), name);
				response->setName(stuffName);
			}
		}
		this->_name = name;
		_hasChanged = true;
	}
}

std::string ModelDataDefinition::getName() const {
	return _name;
}

std::string ModelDataDefinition::getClassname() const {
	return _typename;
}

void ModelDataDefinition::InitBetweenReplications(ModelDataDefinition* modeldatum) {
	modeldatum->_parentModel->getTracer()->trace("Initing " + modeldatum->getClassname() + " \"" + modeldatum->getName() + "\"", Util::TraceLevel::L8_detailed); //std::to_string(component->_id));
	try {
		modeldatum->_initBetweenReplications();
	} catch (const std::exception& e) {
		modeldatum->_parentModel->getTracer()->traceError(e, "Error initing modeldatum " + modeldatum->show());
	};
}

ModelDataDefinition* ModelDataDefinition::LoadInstance(Model* model, std::map<std::string, std::string>* fields, bool insertIntoModel) {
	std::string name = "";
	if (insertIntoModel) {
		// extracts the name from the fields even before "_laodInstance" and even before construct a new ModelDataDefinition in such way when constructing the ModelDataDefinition, it's done with the correct name and that correct name is show in trace
		std::map<std::string, std::string>::iterator it = fields->find("name");
		if (it != fields->end())
			name = (*it).second;
	}
	ModelDataDefinition* newElement = new ModelDataDefinition(model, "ModelDataDefinition", name, insertIntoModel);
	try {
		newElement->_loadInstance(fields);
	} catch (const std::exception& e) {

	}
	return newElement;
}

ModelDataDefinition* ModelDataDefinition::NewInstance(Model* model, std::string name) {
	//nobody will never call this static method. Only its subclasses
	assert(false);
	return nullptr;
}

std::map<std::string, std::string>* ModelDataDefinition::SaveInstance(ModelDataDefinition* modeldatum) {
	std::map<std::string, std::string>* fields; // = new std::list<std::string>();
	try {
		fields = modeldatum->_saveInstance(modeldatum->_getSaveDefaultsOption());
	} catch (const std::exception& e) {
		//modeldatum->_model->getTrace()->traceError(e, "Error saving anElement " + modeldatum->show());
	}
	return fields;
}

bool ModelDataDefinition::Check(ModelDataDefinition* modeldatum, std::string* errorMessage) {
	//    modeldatum->_model->getTraceManager()->trace(Util::TraceLevel::L9_mostDetailed, "Checking " + modeldatum->_typename + ": " + modeldatum->_name); //std::to_string(modeldatum->_id));
	bool res = false;
	Util::IncIndent();
	{
		try {
			res = modeldatum->_check(errorMessage);
			if (!res) {
				//                modeldatum->_model->getTraceManager()->trace(Util::TraceLevel::errors, "Error: Checking has failed with message '" + *errorMessage + "'");
			}
		} catch (const std::exception& e) {
			//            modeldatum->_model->getTraceManager()->traceError(e, "Error verifying modeldatum " + modeldatum->show());
		}
	}
	Util::DecIndent();
	return res;
}

void ModelDataDefinition::CreateInternalData(ModelDataDefinition* modeldatum) {
	try {
		Util::IncIndent();
		modeldatum->_createInternalData();
		Util::DecIndent();
	} catch (const std::exception& e) {
		//modeldatum->...->_model->getTraceManager()->traceError(e, "Error creating elements of modeldatum " + modeldatum->show());
	};
}

void ModelDataDefinition::_createInternalData() {

}

void ModelDataDefinition::setReportStatistics(bool reportStatistics) {
	if (_reportStatistics != reportStatistics) {
		this->_reportStatistics = reportStatistics;
		_hasChanged = true;
	}
}

bool ModelDataDefinition::isReportStatistics() const {
	return _reportStatistics;
}
