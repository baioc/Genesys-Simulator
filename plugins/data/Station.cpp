/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   Station.cpp
 * Author: rlcancian
 *
 * Created on 03 de Junho de 2019, 15:12
 */

#include "Station.h"
#include "../../kernel/simulator/Entity.h"
#include "../../kernel/simulator/Model.h"
#include "../../kernel/simulator/Attribute.h"

#ifdef PLUGINCONNECT_DYNAMIC

extern "C" StaticGetPluginInformation GetPluginInformation() {
	return &Station::GetPluginInformation;
}
#endif

ModelDataDefinition* Station::NewInstance(Model* model, std::string name) {
	return new Station(model, name);
}

Station::Station(Model* model, std::string name) : ModelDataDefinition(model, Util::TypeOf<Station>(), name) {
}

Station::~Station() {
	//_parentModel->elements()->remove(Util::TypeOf<StatisticsCollector>(), _cstatNumberInStation);
	//_parentModel->elements()->remove(Util::TypeOf<StatisticsCollector>(), _cstatTimeInStation);
}

std::string Station::show() {
	std::string msg = ModelDataDefinition::show() + ",enterIntoStationComponent=";
	if (_enterIntoStationComponent == nullptr)
		msg += "NULL";
	else
		msg += _enterIntoStationComponent->getName();
	return msg;
}

void Station::initBetweenReplications() {
	_cstatNumberInStation->getStatistics()->getCollector()->clear();
	_cstatTimeInStation->getStatistics()->getCollector()->clear();

}

void Station::enter(Entity* entity) {
	std::string attributeName = "Entity.ArrivalAt" + this->getName();
	trimwithin(attributeName);
	entity->setAttributeValue(attributeName, _parentModel->getSimulation()->getSimulatedTime());
	entity->setAttributeValue("Entity.Station", _id);
	_numberInStation++;
	if (_reportStatistics)
		this->_cstatNumberInStation->getStatistics()->getCollector()->addValue(_numberInStation);
}

void Station::leave(Entity* entity) {
	std::string attributeName = "Entity.ArrivalAt" + this->getName();
	trimwithin(attributeName);
	double arrivalTime = entity->getAttributeValue(attributeName);
	double timeInStation = _parentModel->getSimulation()->getSimulatedTime() - arrivalTime;
	entity->setAttributeValue("Entity.Station", 0.0);
	_numberInStation--;
	if (_reportStatistics) {
		_cstatNumberInStation->getStatistics()->getCollector()->addValue(_numberInStation);
		_cstatTimeInStation->getStatistics()->getCollector()->addValue(timeInStation);
		if (entity->getEntityType()->isReportStatistics())
			entity->getEntityType()->addGetStatisticsCollector(entity->getEntityTypeName() + ".TimeInStations")->getStatistics()->getCollector()->addValue(timeInStation); // @TODO: should check if entitytype reports (?)
	}
}

void Station::setEnterIntoStationComponent(ModelComponent* _enterIntoStationComponent) {
	this->_enterIntoStationComponent = _enterIntoStationComponent;
}

ModelComponent* Station::getEnterIntoStationComponent() const {
	return _enterIntoStationComponent;
}

PluginInformation* Station::GetPluginInformation() {
	PluginInformation* info = new PluginInformation(Util::TypeOf<Station>(), &Station::LoadInstance, &Station::NewInstance);
	std::string help = "The Station module defines a station (or a set of stations) corresponding to a physical or logical location where processing occurs.";
	help += " If the Station module defines a station set, it is effectively defining multiple processing locations.";
	help += " The station (or each station within the defined set) has a matching Activity Area that is used to report all times and costs accrued by the entities in this station.";
	help += " This Activity Area’s name is the same as the station.";
	help += " If a parent Activity Area is defined, then it also accrues any times and costs by the entities in this station.";
	help += " TYPICAL USES: (1) Defining a lathe area; (2) Defining a set of toll booths; (3) Defining a food preparation area";
	info->setDescriptionHelp(help);
	return info;
}

ModelDataDefinition* Station::LoadInstance(Model* model, std::map<std::string, std::string>* fields) {
	Station* newElement = new Station(model);
	try {
		newElement->_loadInstance(fields);
	} catch (const std::exception& e) {

	}
	return newElement;
}

bool Station::_loadInstance(std::map<std::string, std::string>* fields) {
	bool res = ModelDataDefinition::_loadInstance(fields);
	if (res) {
		try {
		} catch (...) {
		}
	}
	return res;
}

std::map<std::string, std::string>* Station::_saveInstance(bool saveDefaultValues) {
	std::map<std::string, std::string>* fields = ModelDataDefinition::_saveInstance(saveDefaultValues); //Util::TypeOf<Station>());
	return fields;
}

bool Station::_check(std::string* errorMessage) {
	/* include attributes needed */
	std::vector<std::string> neededNames = {"Entity.Station"};
	neededNames.insert(neededNames.begin(), "Entity.ArrivalAt" + this->getName());
	std::string neededName;
	for (unsigned int i = 0; i < neededNames.size(); i++) {
		neededName = neededNames[i];
		if (_parentModel->getDataManager()->getDataDefinition(Util::TypeOf<Attribute>(), neededName) == nullptr) {
			new Attribute(_parentModel, neededName);
		}
	}
	*errorMessage += "";
	return true;
}

void Station::_createInternalData() {
	if (_reportStatistics) {
		if (_cstatNumberInStation == nullptr) {
			_cstatNumberInStation = new StatisticsCollector(_parentModel, getName() + "." + "NumberInStation", this);
			_cstatTimeInStation = new StatisticsCollector(_parentModel, getName() + "." + "TimeInStation", this);
			_internalData->insert({"NumberInStation", _cstatNumberInStation});
			_internalData->insert({"TimeInStation", _cstatTimeInStation});
			//
			// include StatisticsCollector needed in EntityType
			std::list<ModelDataDefinition*>* enttypes = _parentModel->getDataManager()->getDataDefinitionList(Util::TypeOf<EntityType>())->list();
			for (ModelDataDefinition* modeldatum : *enttypes) {
				if (modeldatum->isReportStatistics())
					static_cast<EntityType*> (modeldatum)->addGetStatisticsCollector(modeldatum->getName() + ".TimeInStations"); // force create this CStat before simulation starts
			}

		}
	} else
		if (_cstatNumberInStation != nullptr) {
		_removeInternalDatas();
	}
}
