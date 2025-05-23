;; XTERM keyboard definitions
;;
;; Map xterm function key escape sequences
;; into the standard slots in function-keymap.
;; Based on the vt100 key and xterm emulation
;; found in the GNU emacs distribution.
;;
;; Written by Udo Munk, Mark Williams Company

;; Don't send the `ti' string when screen is cleared.
(setq reset-terminal-on-clear nil)

(require 'keypad)

(defvar CSI-map nil
  "The CSI-map maps the CSI function keys on the xterm keyboard.
The CSI keys are the movement keys.")

(setq-default CSI-mapped nil)

(if (not CSI-map)
    (progn
     (setq-default CSI-mapped t)
     (setq CSI-map (lookup-key global-map "\e["))
     (if (not (keymapp CSI-map))
	 (setq CSI-map (make-sparse-keymap)))  ;; <ESC>[ commands

     (setup-terminal-keymap CSI-map
	    '(("A" . ?u)	   ; up arrow (previous-line)
	      ("B" . ?d)	   ; down-arrow (next-line)
	      ("C" . ?r)	   ; right-arrow (forward-char)
	      ("D" . ?l)           ; left-arrow (backward-char)
              ("6~" . ?N)          ; page-down (scroll-down)
              ("5~" . ?P)          ; page-up (scroll-up)
      ))))

(defun arrow-on () (CSI-map))

(defun enable-arrow-keys ()
  "Enable the use of the xterm arrow keys for cursor motion.
Because of the nature of the xterm emulation, this unavoidably breaks
the standard Emacs command ESC [; therefore, it is not done by default,
but only if you give this command."
  (interactive)
  (global-set-key "\e[" CSI-map)      ; map the escape sequences
)
