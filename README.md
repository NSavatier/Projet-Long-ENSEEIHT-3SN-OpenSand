
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

Then, you'll need to compile the sources, a method can be found on the [OpenSAND wiki,](https://wiki.net4sat.org/doku.php?id=opensand:manuals:compilation_manual:index) but you can also use my scripts to do so (I recommend to use them for simplicity's sake).

Once you have compiled the sources, you should have various .deb packages.
You can install them directly using you preferred packet manager (such as apt).

*(note to myself : incude said scripts in the repo )*

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

## Work Technical Description

*(description to be extended in the near future)*

In the meantime, you can have a look [on a report I did on the subject](https://docs.google.com/document/d/1ub_hdDlMNW_xi6T9Ws51WGOzviqY1qHeX4DEuUJeD-E/edit?usp=sharing).

