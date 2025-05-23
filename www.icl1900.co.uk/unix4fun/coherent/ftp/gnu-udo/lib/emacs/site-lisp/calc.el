;; Integer calculator mode
;;
;; Supports the operators +, -, *, / and % (reminder)
;; Commands:
;; c       clear the stack
;; =       print the value at the top of the stack
;; p       print the entire stack contents
;;
;; Taken from the book:
;;    Learning GNU Emacs
;;    Debra Cameron and Bill Rosenblatt
;;    O'Reilly & Associates, Inc.

(defvar calc-mode-map nil
  "Local keymap for calculator mode buffers.")

; set up the calculator mode keymap with
; C-j (linefeed) as "eval" key
(if calc-mode-map
    nil
  (setq calc-mode-map (make-sparse-keymap))
  (define-key calc-mode-map "\C-j" 'calc-eval))

(defconst integer-regexp "-?[0-9]+"
  "Regular expression for recognizing integers.")

(defconst operator-regexp "[-+*/%]"
  "Regular expression for recognizing operators.")

(defconst command-regexp "[c=ps]"
  "Regular expression for recognizing commands.")

(defconst whitespace "[ \t]"
  "Regular expression for recognizing whitespace.")

;; beep and print an error message
(defun error-message (str)
  (beep)
  (message str)
  nil)

;; stack functions
(defun push (num)
  (if (integerp num)
      (setq stack (cons num stack))))

(defun top ()
  (if (not stack)
      (error-message "stack empty.")
    (car stack)))

(defun pop ()
  (let ((val (top)))
    (if val
	(setq stack (cdr stack)))
    val))

;; functions for user commands
(defun print-stack ()
  "Print entire contents of stack, from top to bottom."
  (if stack
      (progn
	(insert "\n")
	(let ((stk stack))
	  (while stack
	    (insert (int-to-string (pop)) " "))
	  (setq stack nil)))
    (error message "stack empty.")))

(defun clear-stack ()
  "Clear the stack."
  (setq stack nil)
  (message "stack cleared."))

(defun command (tok)
  "Given command token, perform the appropriate action."
  (cond ((equal tok "c")
	 (clear-stack))
	((equal tok "=")
	 (insert "\n" (int-to-string (top))))
	((equal tok "p")
	 (print stack))
	(t
	 (message (concat "invalid command: " tok)))))

(defun operate (tok)
  "Given an arithmetic operator (as string), pop two numbers
off the stack, perform operation tok (given as string), push
the result onto the stack."
  (let ((op1 (pop))
	(op2 (pop)))
    (push (funcall (read tok) op2 op1))))

(defun push-number (tok)
  "Given a number (as string), push it (as interger)
onto the stack."
  (push (string-to-int tok)))

(defun invalid-tok (tok)
  (error-message (concat "Invalid token: " tok))
  (sit-for 0))      ;make sure message is displayed

(defun next-token ()
  "Pick up the next token, based in regexp search.
As side effects, advance point one past the token,
and set name of function to use to process the token."
  (let (tok)
    (cond ((looking-at integer-regexp)
	   (goto-char (match-end 0))
	   (setq proc-fun 'push-number))
	  ((looking-at operator-regexp)
	   (forward-char 1)
	   (setq proc-fun 'operate))
	  ((looking-at command-regexp)
	   (forward-char 1)
	   (setq proc-fun 'command))
	  (t
	   (setq proc-fun 'invalid-tok)))
    ;; pick up token and advance past it (and past whitespace)
    (setq tok (buffer-substring (match-beginning 0) (point)))
    (if (looking-at whitespace)
	(goto-char (match-end 0)))
    tok))

(defun calc-eval ()
  "Main evalution function for calculator mode.
Process all tokens on an input line."
  (interactive)
  (beginning-of-line)
  (while (not (eolp))
    (let ((tok (next-token)))
      (funcall proc-fun tok)))
  (insert "\n"))

(defun calc-mode ()
  "Calculator mode. 6-digit precision, postfix notation.
Understands the arithmetic operators +, -, *, / and %,
plus the following commands:
    c   clear stack
    =   print top of stack
    p   print entire stack contents (top to bottom)
Linefeed (C-j) is bound to an evaluation function that
will evaluate everything on the current line. No
whitespace is necessary, expect to seperate number."
  (interactive)
  (pop-to-buffer "*Calc*" nil)
  (kill-all-local-variables)
  (make-local-variable 'stack)
  (setq stack nil)
  (make-local-variable 'proc-fun)
  (setq major-mode 'calc-mode)
  (setq mode-name "Calculator")
  (use-local-map calc-mode-map))