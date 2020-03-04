#!/bin/bash

#this script packages the opensand sources (located in the Parent folder of this script)
#into an archive named opensand.tar.gz, and moves it in the current folder
current_folder=$(pwd)
cd ..
tar -vczf $current_folder/opensand.tar.gz . --exclude "compilation-scripts" --exclude ".git"

