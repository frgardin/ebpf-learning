# Pods

Pods, or groups of containers, can group together container images developed by•
different teams into a single deployable unit.

# ReplicaSet’s

A ReplicaSet's purpose is to maintain a stable set of replica Pods running at any given time. Usually, you define a Deployment and let that Deployment manage ReplicaSets automatically.

A ReplicaSet's purpose is to maintain a stable set of replica Pods running at any given time. As such, it is often used to guarantee the availability of a specified number of identical Pods.

# K8’s Components

Using command:

```bash

> kubectl get componentstatuses
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