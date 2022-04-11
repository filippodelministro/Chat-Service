# Reti_Project
###### Network's Course Project for _University of Pisa_
The project's goal is to create a working network chat service in which users can send/receive messages and share files using chat.
Users cn

Users can ask the server to open a new chat with a specific user,

---


#### DEVICE 

* `signup <srv_port> <username> <password>`: Allow to register a new account in the server service listening at `srv_port`; return error and exit if `username` is already used by another user.


* `in <srv_port> <username> <password>`: Login `username` if `username` and `password` are correct.

**Note:** The following can be used only if user is connected

* `hanging`: List all users who send message to the user while offline.


* `show <username>`: Show messages sent by `username`.

* `chat <username>`: Start a new chat chat with `username` if exists.

* `share <file_name>`: Send `name_file` to connected user(s).

* `out`: Logout and disconnect.

---

#### SERVER

1. `help`: prompt an help command.

2. `list`: list all the connected user with their login time.

3. `esc`: shutdown server
_Note:_ this does not make users offline; from now on they can just use already existing chat, and not creating new ones.

---

#### Structure of the project 
The projet has two main file which both inlcude an "all.h" file: here all defined all the libraries and data structure for socket programming.
The two main file are
    - server.c 
    - dev.c

Both of this file are devided in few sector:
    - DECLARATION   :a generic list of data structure and variables declaration
    - COMMAND       :command list that can be used by user (different from server to device)
    - UTILTY        :utility function (such as in/out function)
    - FUNCTION      :function used by main function (can not be used by the user)
    - MAIN          :main program
