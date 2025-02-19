#!/bin/sh

sudo apt-get update
sudo apt-get upgrade
sudo apt-get install --fix-missing

sudo apt install terminator
sudo apt install konsole
sudo apt install libncurses-dev
sudo apt install libcjson-dev

echo "Please specify an installation folder:"
read install_folder

if [ ! -d "$install_folder" ]; then
  echo "Error: the specified installation folder does not exist."
  exit 1
fi

cd "$install_folder"
mkdir -p fastdds

curl -o fastdds.tgz 'https://www.eprosima.com/component/ars/item/eProsima_Fast-DDS-v3.1.1-Linux.tgz?format=tgz&category_id=7&release_id=159&Itemid=0'

tar -xvzf ./fastdds.tgz -C fastdds
cd fastdds/fastdds
sudo ./install.sh

