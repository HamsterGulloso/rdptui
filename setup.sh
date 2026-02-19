if [ -d "termbox2" ]; then
    git -C ./termbox2/ pull
else
    git clone git@github.com:termbox/termbox2
fi
