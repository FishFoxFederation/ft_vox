#!/bin/bash

if [ ! -d external/cppVulkanAPI ]
then
	echo "cloning 'git@github.com:SaumonDesMers/cppVulkanAPI.git' in external/cppVulkanAPI"

	git clone git@github.com:SaumonDesMers/cppVulkanAPI.git external/cppVulkanAPI
fi