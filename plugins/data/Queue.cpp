/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Queue.cpp
 * Author: rafael.luiz.cancian
 * 
 * Created on 21 de Agosto de 2018, 17:12
 */

#include "Queue.h"
#include "../../kernel/simulator/Model.h"
#include "../../kernel/simulator/Attribute.h"

#ifdef PLUGINCONNECT_DYNAMIC

extern "C" StaticGetPluginInformation GetPluginInformation() {
	return &Queue::GetPluginInformation;
}
#endif

ModelDataDefinition* Queue::NewInstance(Model* model, std::string name) {
	return new Queue(model, name);
}

Queue::Queue(Model* model, std::string name) : ModelDataDefinition(model, Util::TypeOf<Queue>(), name) {
}

Queue::~Queue() {
	//_parentModel->elements()->remove(Util::TypeOf<StatisticsCollector>(), _cstatNumberInQueue);
	//_parentModel->elements()->remove(Util::TypeOf<StatisticsCollector>(), _cstatTimeInQueue);
}

std::string Queue::show() {
	return ModelDataDefinition::show() +
			",waiting=" + this->_list->show();
}

void Queue::insertElement(Waiting* modeldatum) {
	_list->insert(modeldatum);
	if (_reportStatistics)
		this->_cstatNumberInQueue->getStatistics()->getCollector()->addValue(_list->size());
}

void Queue::removeElement(Waiting* modeldatum) {
	double tnow = _parentModel->getSimulation()->getSimulatedTime();
	_list->remove(modeldatum);
	if (_reportStatistics) {
		this->_cstatNumberInQueue->getStatistics()->getCollector()->addValue(_list->size());
		double timeInQueue = tnow - modeldatum->getTimeStartedWaiting();
		this->_cstatTimeInQueue->getStatistics()->getCollector()->addValue(timeInQueue);
	}
}

void Queue::initBetweenReplications() {
	this->_list->clear();
}

unsigned int Queue::size() {
	return _list->size();
}

Waiting* Queue::first() {
	return _list->front();
}

Waiting* Queue::getAtRank(unsigned int rank) {
	return _list->getAtRank(rank);
}

void Queue::setAttributeName(std::string _attributeName) {
	this->_attributeName = _attributeName;
}

std::string Queue::getAttributeName() const {
	return _attributeName;
}

void Queue::setOrderRule(OrderRule _orderRule) {
	this->_orderRule = _orderRule;
}

Queue::OrderRule Queue::getOrderRule() const {
	return _orderRule;
}

double Queue::sumAttributesFromWaiting(Util::identification attributeID) {
	double sum = 0.0;
	for (Waiting* waiting : *_list->list()) {
		sum += waiting->getEntity()->getAttributeValue(attributeID);
	}
	return sum;
}

double Queue::getAttributeFromWaitingRank(unsigned int rank, Util::identification attributeID) {
	Waiting* wait = _list->getAtRank(rank);
	if (wait != nullptr) {
		return wait->getEntity()->getAttributeValue(attributeID);
	}
	return 0.0;
}

PluginInformation* Queue::GetPluginInformation() {
	PluginInformation* info = new PluginInformation(Util::TypeOf<Queue>(), &Queue::LoadInstance, &Queue::NewInstance);
	return info;
}

ModelDataDefinition* Queue::LoadInstance(Model* model, std::map<std::string, std::string>* fields) {
	Queue* newElement = new Queue(model);
	try {
		newElement->_loadInstance(fields);
	} catch (const std::exception& e) {

	}
	return newElement;
}

bool Queue::_loadInstance(std::map<std::string, std::string>* fields) {
	bool res = ModelDataDefinition::_loadInstance(fields);
	if (res) {
		try {
			this->_attributeName = LoadField(fields, "attributeName", DEFAULT.attributeName);
			this->_orderRule = static_cast<OrderRule> (LoadField(fields, "orderRule", static_cast<int> (DEFAULT.orderRule)));
		} catch (...) {
		}
	}
	return res;
}

std::map<std::string, std::string>* Queue::_saveInstance(bool saveDefaultValues) {
	std::map<std::string, std::string>* fields = ModelDataDefinition::_saveInstance(saveDefaultValues); //Util::TypeOf<Queue>());
	SaveField(fields, "orderRule", static_cast<int> (this->_orderRule), static_cast<int> (DEFAULT.orderRule), saveDefaultValues);
	SaveField(fields, "attributeName", this->_attributeName, DEFAULT.attributeName, saveDefaultValues);
	return fields;
}

bool Queue::_check(std::string* errorMessage) {
	return _parentModel->getDataManager()->check(Util::TypeOf<Attribute>(), _attributeName, "AttributeName", false, errorMessage);
}

void Queue::_createInternalData() {
	if (_reportStatistics) {
		if (_cstatNumberInQueue == nullptr) {
			_cstatNumberInQueue = new StatisticsCollector(_parentModel, getName() + "." + "NumberInQueue", this); /* @TODO: ++ WHY THIS INSERT "DISPOSE" AND "10ENTITYTYPE" STATCOLL ?? */
			_cstatTimeInQueue = new StatisticsCollector(_parentModel, getName() + "." + "TimeInQueue", this);
			_internalData->insert({"NumberInQueue", _cstatNumberInQueue});
			_internalData->insert({"TimeInQueue", _cstatTimeInQueue});
		}
	} else if (_cstatNumberInQueue != nullptr) {
		// @TODO: remove
		_removeInternalDatas();
		//_internelElements->remove(_cstatNumberInQueue);
		//_internelElements->remove(_cstatTimeInQueue);
		//_cstatNumberInQueue->~StatisticsCollector();
		//_cstatTimeInQueue->~StatisticsCollector();
	}
}

ParserChangesInformation * Queue::_getParserChangesInformation() {
	ParserChangesInformation* changes = new ParserChangesInformation();
	//changes->getProductionToAdd()->insert(...);
	//changes->getTokensToAdd()->insert(...);
	return changes;
}