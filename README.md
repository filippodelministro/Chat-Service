# Chat Service
###### Network's Course Project for _@University of Pisa_
The project's aim is to create a working network chat service in which users can send/receive messages and share files using chat.
Users can ask for a new chat to the server which manage all the online users, linking them for the first time; once linked, devices can chat and share file with other users, even in groupchats.
Check the [project requirements](docs/chat_service.pdf) for details. 


---

#### DEVICE

* `signup <srv_port> <username> <password>`: Allow to register a new account in the server service listening at `srv_port`; return error and exit if `username` is already used by another user.


* `in <srv_port> <username> <password>`: Login `username` if `username` and `password` are correct.

**Note:** The following can be used only if user is connected

* `list`: show all users to send messages

* `hanging`: List all users who send message while user were offline

* `show <username>`: Show messages sent by `username`

* `chat <username>`: Start a new chat chat with `username` if exists
    * `<msg> + ENTER`: send message
    * `\u`: show other users
    * `\a <user>`: add user in chat if online and noy busy
    * `\s <filename>`: share `<filename>` if exists (not allowed in groupchats)
    * `\c`: clear chat history
    * `\q`: quit chat 

* `out`: Logout and disconnect.

---

#### SERVER

1. `help`: prompt an help command.

2. `list`: list all the connected user with their login time.

3. `esc`: shutdown server
_Note:_ this does not make users offline; from now on they can just use already existing chat, and not creating new ones.

---

#### USAGE
You can use the `makefile`, or use `./exec.sh` to compile and execute server and three devices on default port.

Devices can be executed with the following syntax:
```
$ ./dev <dev_port>
```

Server can be executed with the following syntax: [using port 4242]

```
$ ./server
```
or the following to execute on custom port:
```
$ ./server <server_port>
```
---
Filippo Del Ministro, 24.06.22