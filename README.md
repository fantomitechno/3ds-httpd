# 3ds-httpd

## A modular web server for your 3ds

Customized from fork

### Path Handlers

#### System Handler

    - Path: /system/
    - Description:
    	/system/exit - Exits app
    	/system/reboot - Reboots system

#### SDMC Handler

    - Path: /
    - Description:
    	Returns content of requested file in SD Card with content-type text/html, if file is missing returns 404. Path starts at SDMC root
