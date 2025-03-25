# Namespaces

Kubernetes uses *namespaces* to organize objects in the cluster.

```bash

$ kubernetes --namespace=mynamespace ...

$ kubernetes -n mynamespace ...

$ kubernetes --all-namespaces ...
```

# Contexts

If you want to change the default namespace, you can use a *context*. 

This gets recorded in a *kubectl* configuration, usually located in *$HOME/.kube/config*.

This configuration file also stores how to both find and authenticate to your cluster. For example, you can create a context with a different default namespace for you kubectl commands using:

```bash
$ kubectl config set-context my-context --namespace=mystuff
```

This creates a new context, but it doesn't actually start using it yet. To use this newly created context, you can run:

```bash
$ kubectl config use-context my-context
```

Contexts can also be used to manage different clusters or different users for authenticating to those clusters using the *--users* or *--clusters* flags with the *set-context* command.

# Viewing Kubernetes API Objects


