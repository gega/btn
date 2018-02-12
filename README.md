# btn
kobo button relay client and server

home automation with kobo wifi.
kobo buttons reused as volume/play/pause buttons for a local mpd server
and also control hue lights

the button events are briefly processed by the kobo (libev+c) and the
processed events sent over the local network (wifi/ethernet) to the
server which performs the necessary actions.
