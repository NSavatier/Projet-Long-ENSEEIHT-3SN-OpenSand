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

bool ConfUpdateXMLParser::modifyBandwidthAndSymbolRateInGlobalConf(spot_id_t spot, tal_id_t gw_id, freq_mhz_t newValue, string rootTag){
    
    bool success = false; //success of modification + write to file
    bool modificationSuccess = false; //success of both modifications
    bool modification1Success = false; //success of bandwidth update
    bool modification2Success = false; //success of symbolRate update

    //check rootTag if one of the two authorized
    if(rootTag != "forward_down_band" && rootTag != "return_up_band") {
        return false;
    }

    try
    {
        std::cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$ Lastest Version of ConfUpdateXMLPareser starting $$$$$$$$$$$$$$$$$$$$$$" << std::endl;

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
                                    double oldValueForBandwidth;
                                    std::cout << "Wanted tag found" << std::endl;
                                    //get bandwidth tag
                                    xmlpp::Node::NodeList list3 = (*iter2)->get_children("bandwidth");
                                    for(xmlpp::Node::NodeList::iterator iter3 = list3.begin(); iter3 != list3.end(); ++iter3)
                                    {
                                        std::cout << (*iter3)->get_name() << std::endl;
                                        //modify tag value to newValue
                                        xmlpp::Element* nodeElement2 = dynamic_cast<xmlpp::Element*>((*iter3));
                                        //get the old bandwidth value
                                        string oldText = nodeElement2->get_child_text()->get_content();
                                        oldValueForBandwidth = std::stod(oldText);
                                        //set the bandwidth new value
                                        nodeElement2->set_child_text(std::to_string(newValue));//TODO potentiellement tres peu robuste ! A ameliorer

                                        modification1Success = true;
                                    }
                                    //get carriers_distribution tag
                                    xmlpp::Node::NodeList list4 = (*iter2)->get_children("carriers_distribution");
                                    //for each carrier_distribution tag (should be only one)
                                    for(xmlpp::Node::NodeList::iterator iter4 = list4.begin(); iter4 != list4.end(); ++iter4)
                                    {
                                        xmlpp::Node::NodeList list5;
                                        if(rootTag == "forward_down_band")
                                        {
                                            //get "down_carriers" children
                                            list5 = (*iter4)->get_children("down_carriers");
                                        }
                                        else //rootTag is "return_up_band"
                                        {
                                            //get "up_carriers" children
                                            list5 = (*iter4)->get_children("up_carriers");
                                        }
                                        //for each of these nodes, get and update their attribute "symbol_rate"
                                        for(xmlpp::Node::NodeList::iterator iter5 = list5.begin(); iter5 != list5.end(); ++iter5)
                                        {
                                            const xmlpp::Element* nodeElement2 = dynamic_cast<const xmlpp::Element*>((*iter5));
                                            const xmlpp::Element::AttributeList& attributes2 = nodeElement2->get_attributes();
                                            for(xmlpp::Element::AttributeList::const_iterator attrIter2 = attributes2.begin(); attrIter2 != attributes2.end(); ++attrIter2) {
                                                xmlpp::Attribute *attribute2 = *attrIter2;
                                                const Glib::ustring namespace_prefix = attribute2->get_namespace_prefix();

                                                //check if current attribute is "symbol_rate"
                                                if (attribute2->get_name() == "symbol_rate") {
                                                    //get the "symbol_rate" attribute value
                                                    string symbol_rate_value = attribute2->get_value();
                                                    double old_symbol_rate_val = std::stod(symbol_rate_value);
                                                    //compute its new value as
                                                    //symbol_rate_newVal = (newValueForBandwidth / oldValueForBandwidth) * old_symbol_rate_val;
                                                    double symbol_rate_new_val = (newValue / oldValueForBandwidth) * old_symbol_rate_val;
                                                    //finally write the new value for the symbol_rate
                                                    string symbol_rate_new_text = std::to_string(symbol_rate_new_val);
                                                    attribute2->set_value(symbol_rate_new_text);
                                                    std::cout << "Symbol rate updated to " << symbol_rate_new_text << std::endl;
                                                    //after at least one attribute has been updated, consider the modification a success
                                                    modification2Success = true;
                                                }//if not, continue looking for attributes
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        //Finally, if all updates could be done, write the new version of the file on the file system
        modificationSuccess = modification1Success && modification2Success;

        if(modificationSuccess){
            //write to file
            parser.get_document()->write_to_file(GLOBAL_CONF_FILE_PATH, "UTF-8");//void function that can raise an exception
            success = true;//if this line is executed (no exception thrown), then everything went fine => success.
        }
    }
    catch(const std::exception& ex)
    {
        std::cout << "Exception caught: " << ex.what() << std::endl;
    }

    return success;
}


bool ConfUpdateXMLParser::modifyForwardBandwidthAndSymbolRateInGlobalConf(spot_id_t spot, tal_id_t gw_id, freq_mhz_t newValue){
    bool value = modifyBandwidthAndSymbolRateInGlobalConf(spot, gw_id, newValue, "forward_down_band");
    return value;
}

bool ConfUpdateXMLParser::modifyReturnBandwidthAndSymbolRateInGlobalConf(spot_id_t spot, tal_id_t gw_id, freq_mhz_t newValue) {
    bool value = modifyBandwidthAndSymbolRateInGlobalConf(spot, gw_id, newValue, "return_up_band");
    return value;
}


