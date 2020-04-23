
$(feh --bg-fill ${HOME}/backgrounds/current;
rm ${HOME}/backgrounds/current;
ln $(update_background) ${HOME}/backgrounds/current;
feh --bg-fill ${HOME}/backgrounds/current) &
xbindkeys &
setxkbmap gb &
compton &
slstatus &

