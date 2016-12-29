import curses
import readline

onionAscii = ['                 .....     \n', "              .'::;.       \n", '             .loo,         \n', '          .. lko.          \n', '           ;.,lc,          \n', "           c'.;:d          \n", "        .;c' .',;l:.       \n", '      ;c:.   ...;;:doc.    \n', "    cl.   .. ..'.lccclko.  \n", "  .d'  ...   ...';xlloodO, \n", "  x' ...   . .'.,,kdddddx0.\n", " ,d ..  ...  ..,';xkxxkkkKl\n", " ,d '. ..   .',.;;kOkkOOOKo\n", "  x.., '   ...:,;c0OOO000K'\n", "  .d..''.  ' .:;lxX0000KX: \n", "   .cc,,;' '..llOXK00KXx.  \n", '      ,:lxdcllk0WXKOd:.    \n', "          ';:cllc;.        \n"]

# list of strings that compose the onion ascii art
host = ""

class ChatUI:
    def __init__(self, stdscr, userlist_width=30):
        curses.use_default_colors()
        for i in range(0, curses.COLORS):
            curses.init_pair(i, i, -1);
        self.stdscr = stdscr
        # self.stdscr.keypad (True)
        self.userlist = []
        self.inputbuffer = ""
        self.linebuffer = []
        self.chatbuffer = []

        # Curses, why must you confuse me with your height, width, y, x
        userlist_hwyx = (curses.LINES - 2, userlist_width - 1, 0, 0)
        chatbuffer_hwyx = (curses.LINES - 2, curses.COLS-userlist_width-1,
                           0, userlist_width + 1)
        chatline_yx = (curses.LINES - 1, 0)
        self.win_userlist = stdscr.derwin(*userlist_hwyx)
        self.win_chatline = stdscr.derwin(*chatline_yx)
        self.win_chatbuffer = stdscr.derwin(*chatbuffer_hwyx)

        self.redraw_ui(None)

    def resize(self):
        """Handles a change in terminal size"""
        u_h, u_w = self.win_userlist.getmaxyx()
        h, w = self.stdscr.getmaxyx()

        self.win_chatline.mvwin(h - 1, 0)
        self.win_chatline.resize(1, w)

        self.win_userlist.resize(h - 2, u_w)
        self.win_chatbuffer.resize(h - 2, w - u_w - 2)

        self.linebuffer = []
        for msg in self.chatbuffer:
            self._linebuffer_add(msg)

        self.redraw_ui(None)

    def redraw_ui(self, curr):
        """Redraws the entire UI"""
        h, w = self.stdscr.getmaxyx()
        u_h, u_w = self.win_userlist.getmaxyx()
        self.stdscr.clear()
        self.stdscr.vline(0, u_w + 1, "|", h - 2)
        self.stdscr.hline(h - 2, 0, "-", w)
        self.stdscr.refresh()

        self.redraw_userlist(curr, host)
        self.redraw_chatbuffer(0)
        self.redraw_chatline()

    def redraw_chatline(self):
        """Redraw the user input textbox"""
        h, w = self.win_chatline.getmaxyx()
        self.win_chatline.clear()
        start = len(self.inputbuffer) - w + 1
        if start < 0:
            start = 0
        self.win_chatline.addstr(0, 0, self.inputbuffer[start:])
        self.win_chatline.refresh()

    def redraw_userlist(self, curr, hostname):
        """Redraw the userlist"""
        global host
        host = hostname
        self.win_userlist.clear()
        h, w = self.win_userlist.getmaxyx()
        for i, name in enumerate(self.userlist):
            if i >= h:
                break
            #name = name.ljust(w - 1) + "|"
            if i == curr:
                self.win_userlist.addstr(i, 0, str(i+1) + '. ' + name[:w - 1], curses.color_pair(1) | curses.A_BOLD)
            else:
                self.win_userlist.addstr(i, 0, str(i+1) + '. ' + name[:w - 1], curses.color_pair(4) | curses.A_BOLD)
                
        # artList = onion.readlines()
        artList = onionAscii
        i = h - 21
        for line in artList:
            self.win_userlist.addstr(i, 0, line , curses.color_pair(4) | curses.A_BOLD)
            i += 1
        # onion.close()
        self.win_userlist.addstr(i+1, int(w/2) - 8, "TORchat Address:" , curses.color_pair(4) | curses.A_BOLD)
        self.win_userlist.addstr(i+2, 3, hostname , curses.color_pair(4) | curses.A_BOLD)
        self.win_userlist.refresh()


    def redraw_chatbuffer(self, color):
        """Redraw the chat message buffer"""
        self.win_chatbuffer.clear()
        h, w = self.win_chatbuffer.getmaxyx()
        j = len(self.linebuffer) - h
        if j < 0:
            j = 0
        for i in range(min(h, len(self.linebuffer))):
            self.win_chatbuffer.addstr(i, 1, self.linebuffer[j], curses.color_pair(color))
            j += 1
        self.win_chatbuffer.refresh()

    def chatbuffer_add(self, msg, color):
        """

        Add a message to the chat buffer, automatically slicing it to
        fit the width of the buffer

        """
        self.chatbuffer.append(msg)
        self._linebuffer_add(msg)
        self.redraw_chatbuffer(color)
        self.redraw_chatline()
        self.win_chatline.cursyncup()

    def _linebuffer_add(self, msg):
        h, w = self.stdscr.getmaxyx()
        u_h, u_w = self.win_userlist.getmaxyx()
        w = w - u_w - 2
        while len(msg) >= w:
            self.linebuffer.append(msg[:w])
            msg = msg[w:]
        if msg:
            self.linebuffer.append(msg)

    def close_ui(self):
        # self.stdscr.keypad(False)
        curses.endwin()

    def prompt(self, msg):
        """Prompts the user for input and returns it"""
        self.inputbuffer = msg
        self.redraw_chatline()
        res = self.wait_input()
        res = res[len(msg):]
        return res

    def wait_input(self, prompt="", completer=None):
        """

        Wait for the user to input a message and hit enter.
        Returns the message

        """
        self.inputbuffer = prompt
        self.redraw_chatline()
        self.win_chatline.cursyncup()
        last = -1
        historyPos = 0
        while last != ord('\n'):
            # commands to execute based on which key
            # tab: complete
            # arrows: iterate history
            last = self.stdscr.getch()
            if last == ord('\n'):
                readline.add_history (self.inputbuffer)
                tmp = self.inputbuffer
                self.inputbuffer = ""
                self.redraw_chatline()
                self.win_chatline.cursyncup()
                return tmp[len(prompt):]
            elif last == ord ('\t'):
                if completer != None:
                    response = completer.complete (self.inputbuffer)
                    if response:
                        self.inputbuffer = response
                else:
                    pass
            elif last == curses.KEY_UP:
                historyPos += 1
                resp = readline.get_history_item (readline.get_current_history_length () - historyPos)
                if resp != None:
                    self.inputbuffer = resp
            elif last == curses.KEY_DOWN:
                historyPos -= 1
                resp = readline.get_history_item (readline.get_current_history_length () - historyPos)
                if resp != None:
                    self.inputbuffer = resp
            elif last == curses.KEY_BACKSPACE or last == 127:
                if len(self.inputbuffer) > len(prompt):
                    self.inputbuffer = self.inputbuffer[:-1]
            elif last == curses.KEY_RESIZE:
                self.resize()
            elif 32 <= last <= 126:
                self.inputbuffer += chr(last)
            self.redraw_chatline()
