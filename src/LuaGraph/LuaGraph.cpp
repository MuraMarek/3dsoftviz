#include "LuaGraph/LuaGraph.h"

#include "LuaGraph/LuaGraphObserver.h"
#include "LuaInterface/LuaInterface.h"

#include "Data/Graph.h"
#include "Importer/GraphOperations.h"

#include <Diluculum/LuaWrappers.hpp>
#include "Diluculum/LuaState.hpp"

#include <sstream>
#include <iostream>

#include <QDebug>

Lua::LuaGraph* Lua::LuaGraph::instance;

Diluculum::LuaValueList luaCallback( const Diluculum::LuaValueList& params )
{
	std::cout << "C callback" << std::endl;
	if ( !Lua::LuaGraph::hasObserver() ) {
		return Diluculum::LuaValueList();
	}
	Lua::LuaGraph::getInstance()->getObserver()->onUpdate();
	return Diluculum::LuaValueList();
}
DILUCULUM_WRAP_FUNCTION( luaCallback )

Lua::LuaGraph::LuaGraph()
{
	nodes = new QMap<qlonglong, Lua::LuaNode*>();
	edges = new QMap<qlonglong, Lua::LuaEdge*>();
	incidences = new QMap<qlonglong, Lua::LuaIncidence*>();
    evoNodes = new QMap<QString, Lua::LuaNode*>();
	observer = NULL;
	Lua::LuaInterface* lua = Lua::LuaInterface::getInstance();
	Diluculum::LuaState* ls = lua->getLuaState();
	( *ls )["graphChangedCallback"] = DILUCULUM_WRAPPER_FUNCTION( luaCallback );
}
Lua::LuaGraphObserver* Lua::LuaGraph::getObserver() const
{
	return observer;
}

void Lua::LuaGraph::setObserver( Lua::LuaGraphObserver* value )
{
	observer = value;
}

void Lua::LuaGraph::clearGraph()
{
	nodes->clear();
	edges->clear();
	incidences->clear();
}

void Lua::LuaGraph::printGraph()
{
	std::cout << "Incidences " << incidences->count() << std::endl;
	for ( QMap<qlonglong, LuaIncidence*>::iterator i = incidences->begin(); i != incidences->end(); ++i ) {
		std::cout << i.key() << " ";
	}
	std::cout << std::endl;
	std::cout << "Nodes " << nodes->count() << std::endl;
	for ( QMap<qlonglong, LuaNode*>::iterator i = nodes->begin(); i != nodes->end(); ++i ) {
		std::cout << i.key() << " ";
	}
	std::cout << std::endl;
	std::cout << "Edges " << edges->count() << std::endl;
	for ( QMap<qlonglong, LuaEdge*>::iterator i = edges->begin(); i != edges->end(); ++i ) {
		std::cout << i.key() << " ";
	}
	std::cout << std::endl;
}

Lua::LuaGraph* Lua::LuaGraph::getInstance()
{
	if ( instance == NULL ) {
		instance = new Lua::LuaGraph();
	}
	return instance;
}

Lua::LuaGraph* Lua::LuaGraph::loadGraph()
{
	std::cout << "loading graph" << std::endl;
	Lua::LuaInterface* lua = Lua::LuaInterface::getInstance();

	Lua::LuaGraph* result = Lua::LuaGraph::getInstance();
	result->clearGraph();

	Diluculum::LuaState* ls = lua->getLuaState();

	Diluculum::LuaValueMap edges = ( *ls )["getGraph"]()[0].asTable();

	for ( Diluculum::LuaValueMap::iterator iterator = edges.begin(); iterator != edges.end(); ++iterator ) {

		qlonglong id = iterator->first.asTable()["id"].asInteger();

		Lua::LuaEdge* edge = new Lua::LuaEdge();
		edge->setId( id );
		edge->setParams( iterator->first.asTable()["params"] );
		if ( iterator->first.asTable()["label"].type() != 0 ) {
			edge->setLabel( QString::fromStdString( iterator->first.asTable()["label"].asString() ) );
		}
		else {
			std::stringstream sstm1;
			sstm1 << "Edge " << id;
			edge->setLabel( QString::fromStdString( sstm1.str() ) );
		}
		result->edges->insert( id, edge );


		Diluculum::LuaValueMap incidences = iterator->second.asTable();
		for ( Diluculum::LuaValueMap::iterator iterator2 = incidences.begin(); iterator2 != incidences.end(); ++iterator2 ) {
			qlonglong id2 = iterator2->first.asTable()["id"].asInteger();
			edge->addIncidence( id2 );
			Lua::LuaIncidence* incidence = new Lua::LuaIncidence();
			incidence->setId( id2 );
			incidence->setParams( iterator2->first.asTable()["params"] );
			if ( iterator2->first.asTable()["label"].type() != 0 ) {
				incidence->setLabel( QString::fromStdString( iterator2->first.asTable()["label"].asString() ) );
			}
			else {
				std::stringstream sstm2;
				sstm2 << "Incid " << id2;
				incidence->setLabel( QString::fromStdString( sstm2.str() ) );
			}
			incidence->setOriented( iterator2->first.asTable()["direction"].type() != 0 );
			if ( incidence->getOriented() ) {
				incidence->setOutGoing( iterator2->first.asTable()["direction"].asString() == "out" );
			}

			qlonglong id3 = iterator2->second.asTable()["id"].asInteger();
			if ( result->nodes->contains( id3 ) ) {
				result->nodes->value( id3 )->addIncidence( id2 );
			}
			else {
				Lua::LuaNode* node = new Lua::LuaNode();
				node->setId( id3 );
				node->setParams( iterator2->second.asTable()["params"] );
				if ( iterator2->second.asTable()["label"].type() != 0 ) {
					node->setLabel( QString::fromStdString( iterator2->second.asTable()["label"].asString() ) );
				}
				else {
					std::stringstream sstm;
					sstm << "Node " << id3;
					node->setLabel( QString::fromStdString( sstm.str() ) );
				}
				node->addIncidence( id2 );
				result->nodes->insert( id3, node );
			}

			incidence->setEdgeNode( id, id3 );
			result->incidences->insert( id2, incidence );
		}
	}

	std::cout << "Node count: " << result->nodes->count() << std::endl;
	return result;
}

Lua::LuaGraph* Lua::LuaGraph::loadEvoGraph( QString repoFilepath )
{
    std::cout << "loading evo graph" << std::endl;
    Lua::LuaInterface* lua = Lua::LuaInterface::getInstance();

    Lua::LuaGraph* result = Lua::LuaGraph::getInstance();
    result->clearGraph();

    Diluculum::LuaState* ls = lua->getLuaState();

    Diluculum::LuaValueMap edges = ( *ls )["getGraph"]()[0].asTable();

    QList<qlonglong> unusedNodes = QList<qlonglong>();

    for ( Diluculum::LuaValueMap::iterator iterator = edges.begin(); iterator != edges.end(); ++iterator ) {
        // cast nastavenia Edge
        qlonglong edgeId = iterator->first.asTable()["id"].asInteger();
        Lua::LuaEdge* edge = new Lua::LuaEdge();
        edge->setId( edgeId );
        edge->setParams( iterator->first.asTable()["params"] );

        if ( iterator->first.asTable()["label"].type() != 0 ) {
            edge->setLabel( QString::fromStdString( iterator->first.asTable()["label"].asString() ) );
        } else {
            qDebug() << "Edge" << edgeId << "neobsahuje LABEL";
        }

        result->edges->insert( edgeId, edge );

        Diluculum::LuaValueMap incidences = iterator->second.asTable();
        for ( Diluculum::LuaValueMap::iterator iterator2 = incidences.begin(); iterator2 != incidences.end(); ++iterator2 ) {

            // cast nastavenia Incidence
            qlonglong incidenceId = iterator2->first.asTable()["id"].asInteger();
            Lua::LuaIncidence* incidence = new Lua::LuaIncidence();
            incidence->setId(incidenceId );
            incidence->setParams( iterator2->first.asTable()["params"] );
            if ( iterator2->first.asTable()["label"].type() != 0 ) {
                incidence->setLabel( QString::fromStdString( iterator2->first.asTable()["label"].asString() ) );
            }
            else {
                qDebug() << "Incidence" << incidenceId << "neobsahuje LABEL";
            }

            incidence->setOriented( iterator2->first.asTable()["direction"].type() != 0 );
            if ( incidence->getOriented() ) {
                incidence->setOutGoing( iterator2->first.asTable()["direction"].asString() == "out" );
            }

            edge->addIncidence( incidenceId );

            // cast nastavenia Node
            qlonglong nodeId = iterator2->second.asTable()["id"].asInteger();

            QString type = QString::fromStdString( iterator2->second.asTable()["params"].asTable()["type"].asString() );

            QString identifier = "";

            if( !QString::compare( type, "directory" ) || !QString::compare( type, "file" ) ) {
                identifier = type + ":" + QString::fromStdString( iterator2->second.asTable()["params"].asTable()["path"].asString() ).replace( repoFilepath + "/", "" );
//                qDebug() << "directory/file =" << identifier;
            }

            if( !QString::compare( type, "globalModule" ) ) {
                if ( iterator2->second.asTable()["label"].type() != 0 ) {
                    identifier = type + ":" + QString::fromStdString( iterator2->second.asTable()["label"].asString() );
//                    qDebug() << "globalModule =" << identifier;
                } else {
                    qDebug() << "Uzol" << nodeId << "neobsahuje LABEL";
                    return NULL;
                }
            }

            if( !QString::compare( type, "function" ) ) {
                QString modulePath = QString::fromStdString( iterator2->second.asTable()["params"].asTable()["modulePath"].asString() ).replace( repoFilepath + "/", "" );
                if ( iterator2->second.asTable()["label"].type() != 0 ) {
                    identifier = type + ":" + modulePath + ":" + QString::fromStdString( iterator2->second.asTable()["label"].asString() );
//                    qDebug() << "function =" << identifier;
                } else {
                    qDebug() << "Uzol" << nodeId << "neobsahuje LABEL";
                    return NULL;
                }
            }
            Lua::LuaNode* node = new Lua::LuaNode();
            node->setId( nodeId );
            node->setParams( iterator2->second.asTable()["params"] );
            node->setLabel( QString::fromStdString( iterator2->second.asTable()["label"].asString() ) );


            if( result->nodes->contains( nodeId ) ) {
                result->nodes->value( nodeId )->addIncidence( incidenceId );
            } else {
                node->setIdentifier( identifier );

//                qDebug() << identifier;
                result->nodes->insert( nodeId, node );
                node->addIncidence( incidenceId );

                if( identifier == "" ) {
                    unusedNodes.append( nodeId );
                }
            }


            incidence->setEdgeNode( edgeId, nodeId );
            result->incidences->insert( incidenceId, incidence );

            /*
            if( identifier != "" ) {
                if( result->nodes->contains( identifier ) ) {
                    result->nodes->value( identifier )->addIncidence( incidenceId );
                } else {
                    node->setIdentifier( identifier );

                    qDebug() << identifier;
                    result->nodes->insert( identifier, node );
                }
            } else {
                unusedNodes.append( node );
            }

            incidence->setEdgeNode( edgeId, nodeId );
            result->incidences->insert( incidenceId, incidence );
            */
        }
    }

    foreach( qlonglong nodeId, unusedNodes ) {
        Lua::LuaNode* node = result->nodes->find( nodeId ).value();
        QString nodeType = QString::fromStdString( node->getParams()["type"].asString() );
        bool isPartOfModule = false;
        foreach( qlonglong incidenceId, node->getIncidences() ) {
            Lua::LuaEdge* edge = result->edges->find( result->incidences->value( incidenceId )->getEdgeNodePair().first ).value();

            Lua::LuaIncidence* incidence = nullptr;
            if( edge->getIncidences().at( 0 ) != incidenceId ) {
                incidence = result->incidences->find( edge->getIncidences().at( 0 ) ).value();
            } else {
                incidence = result->incidences->find( edge->getIncidences().at( 1 ) ).value();
            }

            Lua::LuaNode* otherNode = result->nodes->find( incidence->getEdgeNodePair().second ).value();
            QString type = QString::fromStdString( otherNode->getParams()["type"].asString() );
            if( !QString::compare( type, "globalModule") ) {
                QString identifier = nodeType + ":" + otherNode->getLabel() + ":" + node->getLabel();
                node->setIdentifier( identifier );
                qDebug() << node->getIdentifier() << "found";
                isPartOfModule = true;
                break;
            }
        }

        if( !isPartOfModule ) {
            QString identifier = nodeType + ":" + node->getLabel();
            node->setIdentifier( identifier );
            qDebug() << node->getIdentifier() << "not found";
        }
    }



    qDebug() << "EvoNode count: " << result->nodes->count();
    return result;
}

Lua::LuaGraph::~LuaGraph()
{
	for ( QMap<qlonglong, Lua::LuaNode*>::iterator i = nodes->begin(); i != nodes->end(); ++i ) {
		delete( *i );
	}

	for ( QMap<qlonglong, Lua::LuaEdge*>::iterator i = edges->begin(); i != edges->end(); ++i ) {
		delete( *i );
	}

	for ( QMap<qlonglong, Lua::LuaIncidence*>::iterator i = incidences->begin(); i != incidences->end(); ++i ) {
		delete( *i );
	}
	delete nodes;
	delete incidences;
	delete edges;
}
QMap<qlonglong, Lua::LuaIncidence*>* Lua::LuaGraph::getIncidences() const
{
	return incidences;
}

bool Lua::LuaGraph::hasObserver()
{
	if ( instance == NULL ) {
		return false;
	}
	return instance->observer != NULL;
}

QMap<qlonglong, Lua::LuaEdge*>* Lua::LuaGraph::getEdges() const
{
	return edges;
}

QMap<qlonglong, Lua::LuaNode*>* Lua::LuaGraph::getNodes() const
{
	return nodes;
}

