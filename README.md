# Overlay-Adapter

## Introduction

overlay-adapter is a userspace filesystem (fuse), it is mainly designed to work with [docker-extra](https://github.com/YLonely/docker) project. It enhances the overlayfs to work with lowerdirs which use shared filesystems, such as NFS, Ceph. It also provides a "copy-up" mechanism to ensure the correctness when a shard storage is used by several docker deamon(shared storage is used to speed up the creation of container in docker-extra project).

## Build
```
./build.sh
```

## Usage
```
./adapter --source=<shared-storage-mount-point> --upper=<upper-dir> <mount-point>
```
The upper-dir here works just like the upperdir in overlayfs.