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

## Work Technical Description

*(description to be extended in the near future)*


