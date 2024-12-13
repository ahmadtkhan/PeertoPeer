# Peer-to-Peer Content Sharing System

## Abstract
This project implements a Peer-to-Peer (P2P) content sharing system using UDP and TCP protocols. The system consists of two main components: a **Peer Client** and an **Index Server**. The Peer Client allows users to register, deregister, list, and download content, while the Index Server manages content indexing and facilitates peer discovery. The project leverages the `make` build system for compilation and is executed using `ifconfig` for network interface configuration. A designated port is required for UDP execution.

---

## Overview

### Features
1. **Peer Client**:
   - Register content to the Index Server.
   - Deregister content or remove all registered files.
   - List available files and download them from peers.
   - Handle upload requests from other peers.

2. **Index Server**:
   - Maintain an index of registered content and associated peers.
   - Facilitate search queries to match peers with requested files.
   - Support file registration and deregistration.
   - Provide a list of unique files available on the network.

---

### Usage Instructions

#### Compilation
- Use the provided `Makefile` to compile the project:
  ```bash
  make
./Index_server <port>
./Peer_client <index_server_ip> <index_server_port>
