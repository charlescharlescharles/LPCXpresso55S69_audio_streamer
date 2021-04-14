import keyboard

def space_callback() : 
    print('you pressed spacebar')

def esc_callback() : 
    print('you pressed esc')
    

def main() : 
    keyboard.add_hotkey('space', space_callback, args=(), suppress=False, timeout=1, trigger_on_release=False)
    keyboard.add_hotkey('esc', esc_callback, args=(), suppress=False, timeout=1,
    trigger_on_release=False)
    while 1 :    
        pass
    

main()
