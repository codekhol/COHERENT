;; Coherent 4.2 PC keyboard definitions
;;
;; Map ansipc function key escape sequences
;; into the standard slots in function-keymap.
;; Based on the vt100 key emulation found in the GNU
;; emacs distribution.
;;
;; Written by Udo Munk, Mark Williams Company

(require 'keypad)

(defvar CSI-map nil
  "The CSI-map maps the CSI function keys on the ansipc keyboard.
The CSI keys are the movement keys.")

(setq-default CSI-mapped nil)

;; Create a few new keypad defaults.
(keypad-default "\C-f" 'info)
(keypad-default "\C-g" 'dired)
(keypad-default "\C-h" 'shell)
(keypad-default "\C-i" 'compile)
(keypad-default "\C-j" 'next-error)
(keypad-default "\C-n" 'scroll-other-window)
(keypad-default "\C-o" 'other-window)

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
	      ("F" . ?\^b)	   ; end (end-of-line)
	      ("H" . ?\^a)         ; home (beginning-of-line)
              ("G" . ?N)           ; page-down (scroll-down)
              ("I" . ?P)           ; page-up (scroll-up)
	      ("M" . ??)	   ; F1 (help)
	      ("Y" . ?\^f)	   ; Shift F1 (info)
	      ("N" . ?\^g)	   ; F2 (dired)
	      ("Z" . ?\^h)	   ; Shift F2 (shell)
	      ("O" . ?\^i)	   ; F3 (compile)
	      ("a" . ?\^j)	   ; shift F3 (next-error)
	      ("T" . ?\^o)	   ; F8 (other-window)
	      ("f" . ?\^n)	   ; Shift F8 (scroll-other-window)
      ))))

(defun arrow-on () (CSI-map))

(defun enable-arrow-keys ()
  "Enable the use of the ansipc arrow keys for cursor motion.
Because of the nature of the ansipc emulation, this unavoidably breaks
the standard Emacs command ESC [; therefore, it is not done by default,
but only if you give this command."
  (interactive)
  (global-set-key "\e[" CSI-map)      ; map the escape sequences
)
