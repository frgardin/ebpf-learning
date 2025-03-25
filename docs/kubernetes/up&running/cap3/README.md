# Pods

Pods, or groups of containers, can group together container images developed by•
different teams into a single deployable unit.

# ReplicaSet’s

A ReplicaSet's purpose is to maintain a stable set of replica Pods running at any given time. Usually, you define a Deployment and let that Deployment manage ReplicaSets automatically.

A ReplicaSet's purpose is to maintain a stable set of replica Pods running at any given time. As such, it is often used to guarantee the availability of a specified number of identical Pods.

# K8’s Components

Using command:

```bash

$ kubectl get componentstatuses
Warning: v1 ComponentStatus is deprecated in v1.19+
NAME                 STATUS    MESSAGE   ERROR
controller-manager   Healthy   ok
scheduler            Healthy   ok
etcd-0               Healthy   ok
```

## Controller-manager

controller-manager is responsible for running various controllers that regulate behavior in the cluster; for example, ensuring that all of the replicas of a service are available and healthy

## Scheduler

The scheduler is responsible for placing different Pods onto nodes in the cluster.

## ETCD

the etcd server is the storage for the cluster all of the API objects are stored.

# Listing Kubernetes Nodes

```bash
$ kubectl get nodes
NAME STATUS ROLES AGE VERSION
kube0 Ready control-plane,master 45d v1.22.4
kube1 Ready <none> 45d v1.22.4
kube2 Ready <none> 45d v1.22.4
kube3 Ready <none> 45d v1.22.4
```

nodes are separated into control-plane nodes that contain containers like the API server,
scheduler, etc., which manage the cluster, and worker nodes where your containers
will run. Kubernetes won’t generally schedule work onto control-plane nodes to
ensure that user workloads don’t harm the overall operation of the cluster.

# Describe command

```
$ kubectl describe nodes kube1

Name:               minikube
Roles:              control-plane
Labels:             beta.kubernetes.io/arch=amd64
                    beta.kubernetes.io/os=linux
                    kubernetes.io/arch=amd64
                    kubernetes.io/hostname=minikube
                    kubernetes.io/os=linux
                    minikube.k8s.io/commit=dd5d320e41b5451cdf3c01891bc4e13d189586ed-dirty
                    minikube.k8s.io/name=minikube
                    minikube.k8s.io/primary=true
                    minikube.k8s.io/updated_at=2025_03_25T07_33_32_0700
                    minikube.k8s.io/version=v1.35.0
                    node-role.kubernetes.io/control-plane=
                    node.kubernetes.io/exclude-from-external-load-balancers=
Annotations:        kubeadm.alpha.kubernetes.io/cri-socket: unix:///var/run/cri-dockerd.sock
                    node.alpha.kubernetes.io/ttl: 0
                    volumes.kubernetes.io/controller-managed-attach-detach: true
CreationTimestamp:  Tue, 25 Mar 2025 07:33:22 -0300
Taints:             <none>
Unschedulable:      false
Lease:
  HolderIdentity:  minikube
  AcquireTime:     <unset>
  RenewTime:       Tue, 25 Mar 2025 07:59:51 -0300
Conditions:
  Type             Status  LastHeartbeatTime                 LastTransitionTime                Reason                       Message
  ----             ------  -----------------                 ------------------                ------                       -------
  MemoryPressure   False   Tue, 25 Mar 2025 07:54:54 -0300   Tue, 25 Mar 2025 07:33:21 -0300   KubeletHasSufficientMemory   kubelet has sufficient memory available
  DiskPressure     False   Tue, 25 Mar 2025 07:54:54 -0300   Tue, 25 Mar 2025 07:33:21 -0300   KubeletHasNoDiskPressure     kubelet has no disk pressure
  PIDPressure      False   Tue, 25 Mar 2025 07:54:54 -0300   Tue, 25 Mar 2025 07:33:21 -0300   KubeletHasSufficientPID      kubelet has sufficient PID available
  Ready            True    Tue, 25 Mar 2025 07:54:54 -0300   Tue, 25 Mar 2025 07:33:23 -0300   KubeletReady                 kubelet is posting ready status
Addresses:
  InternalIP:  192.168.49.2
  Hostname:    minikube
Capacity:
  cpu:                12
  ephemeral-storage:  50206240Ki
  hugepages-1Gi:      0
  hugepages-2Mi:      0
  memory:             16240716Ki
  pods:               110
Allocatable:
  cpu:                12
  ephemeral-storage:  50206240Ki
  hugepages-1Gi:      0
  hugepages-2Mi:      0
  memory:             16240716Ki
  pods:               110
System Info:
  Machine ID:                 6492de64d1494e78a979c8d0f8c861b5
  System UUID:                89d5f21d-f842-477f-89bd-41c4737aabed
  Boot ID:                    aeba137a-2055-4e45-9643-c722f7612f5d
  Kernel Version:             6.11.0-19-generic
  OS Image:                   Ubuntu 22.04.5 LTS
  Operating System:           linux
  Architecture:               amd64
  Container Runtime Version:  docker://27.4.1
  Kubelet Version:            v1.32.0
  Kube-Proxy Version:         v1.32.0
PodCIDR:                      10.244.0.0/24
PodCIDRs:                     10.244.0.0/24
Non-terminated Pods:          (7 in total)
  Namespace                   Name                                CPU Requests  CPU Limits  Memory Requests  Memory Limits  Age
  ---------                   ----                                ------------  ----------  ---------------  -------------  ---
  kube-system                 coredns-668d6bf9bc-qwpm6            100m (0%)     0 (0%)      70Mi (0%)        170Mi (1%)     26m
  kube-system                 etcd-minikube                       100m (0%)     0 (0%)      100Mi (0%)       0 (0%)         26m
  kube-system                 kube-apiserver-minikube             250m (2%)     0 (0%)      0 (0%)           0 (0%)         26m
  kube-system                 kube-controller-manager-minikube    200m (1%)     0 (0%)      0 (0%)           0 (0%)         26m
  kube-system                 kube-proxy-q2mjs                    0 (0%)        0 (0%)      0 (0%)           0 (0%)         26m
  kube-system                 kube-scheduler-minikube             100m (0%)     0 (0%)      0 (0%)           0 (0%)         26m
  kube-system                 storage-provisioner                 0 (0%)        0 (0%)      0 (0%)           0 (0%)         26m
Allocated resources:
  (Total limits may be over 100 percent, i.e., overcommitted.)
  Resource           Requests    Limits
  --------           --------    ------
  cpu                750m (6%)   0 (0%)
  memory             170Mi (1%)  170Mi (1%)
  ephemeral-storage  0 (0%)      0 (0%)
  hugepages-1Gi      0 (0%)      0 (0%)
  hugepages-2Mi      0 (0%)      0 (0%)
Events:
  Type    Reason                   Age   From             Message
  ----    ------                   ----  ----             -------
  Normal  Starting                 26m   kube-proxy       
  Normal  Starting                 26m   kubelet          Starting kubelet.
  Normal  NodeAllocatableEnforced  26m   kubelet          Updated Node Allocatable limit across pods
  Normal  NodeHasSufficientMemory  26m   kubelet          Node minikube status is now: NodeHasSufficientMemory
  Normal  NodeHasNoDiskPressure    26m   kubelet          Node minikube status is now: NodeHasNoDiskPressure
  Normal  NodeHasSufficientPID     26m   kubelet          Node minikube status is now: NodeHasSufficientPID
  Normal  RegisteredNode           26m   node-controller  Node minikube event: Registered Node minikube in Controller
```


# Cluster Components

One of the interesting aspects of Kubernetes is that many of the components that
make up the Kubernetes cluster are actually deployed using Kubernetes itself.

All of these components run in the kube-system namespace.

## Kubernetes Proxy

The Kubernetes proxy is responsible for routing network traffic to load-balanced
services in the Kubernetes cluster.

It is present on every node in the cluster.

```bash
kubectl get daemonSets --namespace=kube-system kube-proxy
NAME         DESIRED   CURRENT   READY   UP-TO-DATE   AVAILABLE   NODE SELECTOR            AGE
kube-proxy   1         1         1       1            1           kubernetes.io/os=linux   45h
```

## Kubernetes DNS

Kubernetes also runs a DNS server, which provides naming and discovery for the
services that are defined in the cluster.

This DNS server also runs as a replicated service on the cluster.

The DNS service is run as a Kubernetes deployment, which manages these replicas (this may also be named coredns or some
other variant):

```bash
$ kubectl get deployments --namespace=kube-system
NAME      READY   UP-TO-DATE   AVAILABLE   AGE
coredns   2/2     2            2           45h
               
$ kubectl get services --namespace=kube-system
NAME       TYPE        CLUSTER-IP   EXTERNAL-IP   PORT(S)                  AGE
kube-dns   ClusterIP   10.96.0.10   <none>        53/UDP,53/TCP,9153/TCP   45h

```

This shows that the DNS service for the cluster has the address 10.96.0.10. If you
log in to a container in the cluster, you’ll see that this has been populated into the /etc/
resolv.conf file for the container.