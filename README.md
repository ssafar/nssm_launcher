A launcher for NSSM the service manager
=======================================

[NSSM](https://nssm.cc/) is a really cool Windows utility. It lets you run processes as services even if they weren't originally designed to be able to do that.

Oddly enough though, although it provides a GUI to add / modify service settings, you still need to launch it from the command line. This project aims to help with this: it's a very simple launcher for NSSM, essentially calling `nssm install` and `nssm edit` for you.

It also lists NSSM-based services that already exist: it looks for services in the registry that have executable names ending with `nssm.exe`.

![Pasted image 20250125155341](https://github.com/user-attachments/assets/f0a83495-5de4-4a68-b31a-7d8dd82f869c)

To use it, you have to have NSSM installed / available already somewhere. If this is your first service that you're installing, you will need to set this in the settings dialog. Otherwise, we'll try to grab a version of NSSM from the actual services that exist already.

![Pasted image 20250125155410](https://github.com/user-attachments/assets/ed0033f9-bf63-4bb0-9e2f-2a291ff062b3)
