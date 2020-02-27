//
// Created by nsavatie on 19/02/2020.
//

#include "ConfUpdateXMLParser.h"
#include <iostream>

/**
 * Constructor
 */
ConfUpdateXMLParser::ConfUpdateXMLParser()
{
}

/**
 * Destructor
 */
ConfUpdateXMLParser::~ConfUpdateXMLParser()
{
}

bool ConfUpdateXMLParser::modifyBandwidthInGlobalConf(spot_id_t spot, tal_id_t gw_id, freq_mhz_t newValue, string rootTag){
    
    bool success = false;

    try
    {
        xmlpp::DomParser parser;
        //parser.set_validate();
        //parser.set_throw_messages(throw_messages);
        //We can have the text resolved/unescaped automatically.
        parser.set_substitute_entities(true);
        parser.parse_file(GLOBAL_CONF_FILE_PATH);
        if(parser)
        {
            //Walk the tree:
            const xmlpp::Node* node = parser.get_document()->get_root_node(); //deleted by DomParser.

            const xmlpp::ContentNode* nodeContent = dynamic_cast<const xmlpp::ContentNode*>(node);
            const xmlpp::TextNode* nodeText = dynamic_cast<const xmlpp::TextNode*>(node);
            const xmlpp::CommentNode* nodeComment = dynamic_cast<const xmlpp::CommentNode*>(node);
            
            std::cout << node->get_name() << std::endl;

            //If the root node is a regular node
            if(!nodeContent && !nodeText && !nodeComment)
            {
                //parse its children
                xmlpp::Node::NodeList list = node->get_children(rootTag);
                for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter)
                {
                    std::cout << (*iter)->get_name() << std::endl;
                    //look for a "spot" element
                    xmlpp::Node::NodeList list2 = (*iter)->get_children("spot");
                    for(xmlpp::Node::NodeList::iterator iter2 = list2.begin(); iter2 != list2.end(); ++iter2)
                    {
                        std::cout << (*iter2)->get_name() << std::endl;

                        //if spot's id and gw is the one wanted, modify its value
                        const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>((*iter2));
                        if(nodeElement){
                            const xmlpp::Element::AttributeList& attributes = nodeElement->get_attributes();
                            //check if the spot id and gw id are the ones wanted
                            bool spot_ok = false;
                            bool gw_ok = false;
                            for(xmlpp::Element::AttributeList::const_iterator iter = attributes.begin(); iter != attributes.end(); ++iter)
                            {
                                const xmlpp::Attribute* attribute = *iter;
                                const Glib::ustring namespace_prefix = attribute->get_namespace_prefix();
                                
                                std::cout << attribute->get_name()  << "  " << attribute->get_value() << std::endl;

                                if(attribute->get_name() == "id" && attribute->get_value() == std::to_string(spot))
                                {
                                    spot_ok = true;
                                }
                                if(attribute->get_name() == "gw" && attribute->get_value() == std::to_string(gw_id))
                                {
                                    gw_ok = true;
                                }

                                if(spot_ok && gw_ok){
                                    std::cout << "Wanted tag found" << std::endl;
                                    //get bandwidth tag
                                    xmlpp::Node::NodeList list3 = (*iter2)->get_children("bandwidth");
                                    for(xmlpp::Node::NodeList::iterator iter3 = list3.begin(); iter3 != list3.end(); ++iter3)
                                    {
                                        std::cout << (*iter3)->get_name() << std::endl;
                                        //modify tag value to newValue
                                        xmlpp::Element* nodeElement2 = dynamic_cast<xmlpp::Element*>((*iter3));
                                        nodeElement2->set_child_text(std::to_string(newValue));//TODO potentiellement tres peu robuste ! A ameliorer                          
                                        //write to file 
                                        parser.get_document()->write_to_file(GLOBAL_CONF_FILE_PATH, "UTF-8");
                                        success = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    catch(const std::exception& ex)
    {
        std::cout << "Exception caught: " << ex.what() << std::endl;
    }
    return success;
}


bool ConfUpdateXMLParser::modifyForwardBandwidthInGlobalConf(spot_id_t spot, tal_id_t gw_id, freq_mhz_t newValue){
    bool value;
    value = modifyBandwidthInGlobalConf(spot, gw_id, newValue, "forward_down_band");
    return value;
}

bool ConfUpdateXMLParser::modifyReturnBandwidthInGlobalConf(spot_id_t spot, tal_id_t gw_id, freq_mhz_t newValue) {
    bool value;
    value = modifyBandwidthInGlobalConf(spot, gw_id, newValue, "return_up_band");
    return value;
}


