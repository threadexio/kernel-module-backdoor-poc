# -*- mode: ruby -*-
# vi: set ft=ruby :

Vagrant.configure("2") do |config|
  config.vm.box = "generic/ubuntu2104"

  config.vm.synced_folder "./", "/mnt", type: "sshfs"

  config.vm.provision "shell", inline: <<-SHELL
     apt-get -y update
     apt-get install -y make build-essential linux-headers-$(uname -r)
  SHELL
end
