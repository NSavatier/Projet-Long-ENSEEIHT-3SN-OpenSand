

## Brief Repository Description
This repository is a Fork of Opensand 5.1.2 , made for a student project at ENSEEIHT.

This project was developed in early 2020 to allow for in-simulation bandwidth modification (i.e. modification of the bandwidth while the satellite link simulation is running), for both forward and return links.

The feature was briefly tested, and seems to do the job in most cases, but it should NOT be considered as a production-worthy development, as it needs more extensive testing and peer-reviewing.

Most of the code (as well as most of the git history) is imported from Opensand Repository , available at https://forge.net4sat.org/opensand/opensand

## OpenSAND brief presentation

> *Description coming from OpenSAND's [original README](https://forge.net4sat.org/opensand/opensand)*

OpenSAND is an user-friendly and efficient tool to emulate satellite
communication systems, mainly DVB-RCS - DVB-S2.

It provides a suitable and simple means for performance evaluation and
innovative access and network techniques validation. Its ability to interconnect
real equipments with real applications provides excellent demonstration means.

The source codes of OpenSAND components is distributed "as is" under the terms
and conditions of the GNU GPLv3 license or the GNU LGPLv3 license.

The original project source and website can be visited at [opensand.org](https://opensand.org)

The opensand repository is available [here](https://forge.net4sat.org/opensand/opensand).

## How to use my work
This repository contains the complete sources of OpenSAND, imported from OpenSAND master branch, as well as my modifications/additions made on it.

To use it, simply download the whole repository, or clone it.

Then, you'll need to compile the sources, a method can be found on the [OpenSAND wiki,](https://wiki.net4sat.org/doku.php?id=opensand:manuals:compilation_manual:index) but you can also use my scripts to do so (I recommend to use them for simplicity's sake). See below for how to use my compilation scripts.

Once you have compiled the sources, you should have various .deb packages.
You can install them directly using you preferred packet manager (such as apt).

##  Compilation using my scripts
### Summary
My custom-made compilation scripts are available on the repository, inside the folder "compilation-scripts".


They are decomposed various bash scripts which allow for a somewhat pain-free compilation of openSAND.


I used these scripts during this project development to ease compilation and deployment, so they should do the job for you.


*(detailed description and re-testing of scripts to be done and redacted here)*


### Package the source files
First of all, you'll need to package the sources into an archive.


A script in provided to do so easily. 
Go into the "compilation-scripts" folder, and run it : 

    cd compilation-scripts
    ./packageSources.sh


### Compile my modified version of OpenSAND using the packages archive
Then, you'll need to compile my modified version of OpenSAND on your system.


To do so, I'm providing the script "cleanAndRecompileAll". 
This script does the following operations : 


 - It **uninstalls all versions of OpenSAND present on your system** (as some OpenSAND packages need to be compiled and installed before the  rest of the compilation can be done, and having other versions of OpenSAND may create issues during the compilation)  
   
  - Then, it **installs the packages required to compile** OpenSAND.
 - It copies opensand sources to **/tmp/opensand_installer_dir** (to avoid read/write rights issues).
 - Then it **compiles OpenSAND** from this folder.
 - Finally, it copies the **built packages** it to a new folder ***./opensand-compiled-packages***.


To run it, simply type : 

    ./cleanAndRecompileAll.sh
  
  
  If any error occurs, you should check the logs in your terminal, to see where the issue comes from.
  
  
  If you want to check the compilation logs, you can find them in  : 
  **/tmp/workspace/src/<module_name>/build.log** (for build errors) 
  and **/tmp/workspace/src/<module_name>/config.log** (for configuration errors)


The whole compilation process can take several minutes (around 10 in my computer), so be patient.


If everything went fine, you should have a final output that looks like : 

![build success image](/docImages/buildSuccess.png)
*Every OpenSand package was built successfully.*


You should now have the OpenSAND compiled packages in the folder "opensand-compiled-packages".


  ![The built packages](/docImages/packages.png)
*The resulting compiled packages.*


Those packages can then be used to install OpenSAND on any system.

If you want an easy way to test and deploy OpenSAND on Docker Containers, I recommand the work of my colleague Martin Frisch (to which I contributed), [OpenSAND-Docker](https://github.com/neuaa/opensand-docker).

# Work Technical Description

## High-Level view of our modifications to OpenSAND

  

The work we did on OpenSand consisted in allowing the update of the forward and return links bandwidth, while a simulation was running.

  

To do so, we implemented some modifications in the OpenSAND code, to make Gateways, Satellites terminals and Satellites listen for TCP messages in a specific port.

  

These messages allow a user to request for a bandwidth update, during a simulation.

  

The format for those messages is the following :

![](https://lh3.googleusercontent.com/OJBXI_Ol_ZVpzV-0Xw94RCFMTDSZgnOKZQvfmrF7CR4NeNP9dXmC9Lk9_ynAXtSGlBEoQlSOIvE2c9sa1lSYO2a0bmZo02LQy_lLs4DnGkzyWt8BvF5RIfzShp2IydD352CciF3d)

As you can see in the image above, requests are composed of 4 fields, separated by ":", that indicate the needed information for a bandwidth update :

-   The first field indicates the **ID of the Spot** for which the bandwidth must be updated
    
-   The second is the **ID of the Gateway**
    
-   The third indicates the **type of the request** (0 for an update of the Forward bandwidth, 1 for the Return).
    
-   The fourth field indicates the **new value of the bandwidth** (in MHz).
    

  

Those messages must be sent to all the systems (GWs, STs, SAT) related to the request, in order to keep the simulation coherent.

  

In order to ease this "message-sending", we wrote a shell script that sends those messages to all currently running systems, so that the bandwidth can be correctly updated.

The general principle of this script can be seen in the image below.

![](https://lh6.googleusercontent.com/bDPJCqtjhyUM2CI30Ye3MOd1oJxIIOMuCJ0WNTGUcs33TsnplLbSC4PGS1GlwatNiHBtpqP_j364TOw3tONOruLPv_BKqyl5nagptdp7b_ZCHEcQ3IPK26BO51dr6bYoi93igaXY)
*An example of usage of the modify_bandwidth.sh script*

  

When a bandwidth update is received by a system (ST, GW, SAT), it is analysed, in order to determine if the message concerns the system or not :

-   a **SAT handles all messages** received,
    
-   a **GW handles only** ones with a **GW_ID matching** its own.
    
-   a **ST handles only** the ones with a **SPOT_ID matching** the SPOT the ST is in, and whose GW_ID matches the ID of the Gateway linked to it (to the considered ST).
    
-  ** In all other cases, the message is dropped (ignored).**
    

  

This behavior is illustrated with the example below.![](https://lh4.googleusercontent.com/PPuYOzvWU67qdJf9MwWXUK9hl3P3b2QvJkhoBRrfEuPwEIK2uVBsPVA-SFU1KWlQPddHrQfWNGU-DSLXvf6k3mpeeRk8XdLJjn0tU-ZMUyWRBmR8IPF8mAOIP782nfnDRcK0eNIE)

An example of how the requests are handled by the different systems.

  

If the message is related to the system, it performs the following operations, in order to update the forward/return link bandwidth :


-   The system updates the value of the bandwidth (and the symbol rate) in its configuration file (located at */etc/opensand/core_global.conf* ).
    
    
-   Then the system is partially reloaded, so that the modifications made in the configuration file can be applied.


The system then continues to work as usual afterwards.


## Detailed implementation of the bandwidth update

  

As said in the "high-level view" of our implementation, two main operations must be performed when a bandwidth update request is received by a system :

-   Update the configuration file with the new value for the bandwidth
    
-   Reset part of the system so that the new value can be used for the current simulation, without interrupting it.
    

  

We will detail how we implemented these two operations in the following part of this document.

  

### Update of the core_global.conf configuration file

  

The bandwidth parameter is described in the core_global.conf file, located in /etc/opensand.

  

More precisely, the lines that describe the forward bandwidth are the following :

(Screenshot)

  

And the ones describing the Return bandwidth are :

(Screenshot)

  

Knowing that, we wrote C++ functions to update these two parts of the configuration file.

These functions are implemented in the TODO_INSERT_EXACT_CLASSNAME_HERE file, located in the TODO_FOLDER_NAME folder.

  

These two functions parse the XML configuration file to look for the field containing the bandwidth, and update its value to the new value contained in the received Bandwidth update Request.

  

The symbol rate associated with this bandwidth is also updated, using the formula :

TODO_FORMULA_HERE

  

#### Why updating the symbol rate ?

Our tests have shown that updating the bandwidth without updating the symbol rate caused the number of carriers for the link to be updated, instead of the width of the currently existing carriers.

  

This behavior was due to how the bandwidth computation is implemented in OpenSAND (more precisely, how the bandwidth parameter loaded from the configuration file is used in the initBand and the computeBandwidth functions in the DvbChannel class).

This behavior was not the one expected by our supervisor, and that's why we decided to update the symbol rate, as a workaround.

  

## Resetting part of the system

### Why do we need to reset each system ?

In a typical OpenSAND use, you use the OpenSAND manager (the GUI) to configure various parameters of your simulation.

This modifies various configuration files of the machine that runs the manager.

When you start the simulation, these configuration files are then deployed on each of the "Daemon Machines" (see image below).

![](https://lh5.googleusercontent.com/sa9rRo2I6spqjtmEpC2s-qurjffbkH5wQzdNv3zkXhdCmsDEpoe1tt21ZOIvujkyJHROt-zoacCm-b6T8ZRl5NRj08eZLtGppHU7zk2OUv9U2USQVcjtIZUW4oNf6s8YJpxzVjUP)

The OpenSAND Daemons then starts each of the systems processes (STs, GWs, SAT…) by launching their executable.


OpenSAND configuration files are then read, and an image of their content is saved in memory (as a DomParser).


This means that the simulation parameters stored in the configuration file are loaded once when the simulation starts, and they are not re-read afterwards.
  
  

This is the reason why we chose to partially reset each system when a bandwidth update request is received : To reload the modified configuration file and make sure that the new value of the bandwidth is used in the current simulation.


(If we did not reload the configuration file, modifying it would have no effect for the current simulation).


### How did we implement this "partial reset" ?
  

As all systems have a different usage of the bandwidth, each of them must be reset in a different way.

To know what to reset, we started to look at what blocks used the bandwidth in each system, and what functions in these blocks used it.

Our analysis determined that the bandwidth was used in the Dvb Block of all systems, and that the parameter was linked to the way Spots were handled in OpenSAND (more details on each of the impacted functions below).


We thus chose to reset only those parts of OpenSAND systems, in order to minimize the impact of our modifications to the general behavior of OpenSAND.


We will now detail how we chose to implement the "reset" in each system.

#### Partial Reset of the Gateways (GW)

  To be detailed
  

#### Partial Reset of the satellite (SAT)

To be detailed
  

#### Partial Reset of the Satellite Terminals (ST)

To be detailed
  
  
  

## Known issues and bugs

As our modifications on OpenSAND (allowing a configuration parameter to be updated during a simulation) are not really something that was planned in the original OpenSAND implementation, they have some side effects on the behavior of OpenSAND.

  

We managed to fix some of the issues our modifications created, but in the end it is far from perfect, and we did not have the time to fix them all (this repository is a student project and we had limited time and resources).

  

So far we have identified the following issues :

  

### Warnings in the OpenSAND manager

  

Some warning messages are logged in the OpenSAND manager when a simulation is loaded and Bandwidth update requests are received.

This is normal, and was done for testing and demonstration purposes.

If this behavior is an issue for you, you can revert it by reverting the commit TODO_COMMIT_NUMBER which added this logging behavior.

  

We chose to display those messages as warnings and not NOTICE or DEBUG, because the base OpenSAND code already logs a whole lot of messages with these logging levels, and our added ones were hard to find in the flood of logging messages not related to our modifications.

  
  

### Some Probes are duplicated when a Bandwidth update is called

  

This issue is due to the fact that we reset parts of the Dvb block when a Bandwidth request is received.

This reset re-instantiates some probes which are then re-registered by the OpenSAND manager.

  
  

### Updating the bandwidth with multiple Gatways causes TODO messages to be rejected

  

This issue is due to the fact that (detail here)

A solution for this issue would be to (detail solution here)

