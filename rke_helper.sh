#!/bin/env bash

### general dependencies ###
apt-get update
apt-get -qq install -y zip net-tools

### install Kubernetes using RKE2 ###
mkdir -p /etc/rancher/rke2
sudo cat << EOF | sudo tee /etc/rancher/rke2/config.yaml
cni:
  - calico
disable:
  - rke2-canal
  - rke2-ingress-nginx
EOF

curl -sfL https://get.rke2.io | INSTALL_RKE2_VERSION=v1.33.1+rke2r1 sh -
systemctl enable rke2-server.service
systemctl start rke2-server.service

### setup $KUBECONFIG environment variable ###
export KUBECONFIG=/.kube/config

mkdir -p /.kube

chown -R root:root /.kube
chmod -R 777 /.kube

ln -s /etc/rancher/rke2/rke2.yaml /.kube/config

# change for all required users
echo -e "\nexport KUBECONFIG=/.kube/config" | tee -a /root/.bashrc > /dev/null
echo -e "\nexport KUBECONFIG=/.kube/config" | tee -a /home/fgardin/.zshrc > /dev/null

### update $PATH ###
export PATH=$PATH:/var/lib/rancher/rke2/bin/
echo -e "\nexport PATH=$PATH" | tee -a /root/.bashrc > /dev/null
echo -e "\nexport PATH=$PATH" | tee -a /home/fgardin/.zshrc > /dev/null