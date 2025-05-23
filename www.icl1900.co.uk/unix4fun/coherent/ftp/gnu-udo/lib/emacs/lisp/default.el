;; Global defaults for Coherent 4.2
;; Written by Udo Munk, Mark Williams Company

;;
;; map help to C-x ? (is C-h by default)
;; map C-h to backward-delete-char-untabify
;; That's much more comfortable IMHO, if you don't like it comment it
;;
(define-key global-map "\C-x?" 'help-command)
(define-key global-map "\C-h" 'backward-delete-char-untabify)

;;
;; some useful defaults
;;
(setq-default default-tab-width 8)
(setq-default case-fold-search nil)	; do not ignore case when searching
(setq-default default-major-mode 'text-mode)

;;
;; you might want to un-comment the following line to activate
;; debugging mode if there are bugs in lisp code
;;
;(setq-default debug-on-error t)

;;
;; Coherent 4.2 specific configuration
;;
(setq-default load-path '("/u1/gnu/lib/emacs/lisp" "/u1/gnu/lib/emacs/site-lisp"))
(setq-default manual-program "/bin/man")

;;
;; try to load a terminal emulation
;;
(progn
  (setq termtype (getenv "TERM"))
  (if (or (equal termtype "ansipc")
	  (equal termtype "ansipc-m")
	  (equal termtype "ansipcolor"))
	  (progn
	    (load "term/ansipc")
	    (enable-arrow-keys))
  )
  (if (equal termtype "xterm")
      (progn
	(load "term/xterm")
	(enable-arrow-keys))
  )
  (if (or (equal termtype "vt100")
          (equal termtype "vt101")
	  (equal termtype "vt102")
	  (equal termtype "vt125")
	  (equal termtype "vt131"))
	  (progn
	    (load "term/vt100")
	    (enable-arrow-keys))
  )
  (if (or (equal termtype "vt200")
	  (equal termtype "vt220")
	  (equal termtype "vt240")
	  (equal termtype "vt300"))
	  (progn
	    (load "term/vt200")
	    (enable-arrow-keys))
  )
)

;;
;; C-mode customization
;;
(setq c-auto-newline nil)
(setq c-indent-level 4)
(setq c-continued-statement-offset 4)
(setq c-argdecl-indent 0)
(setq c-brace-offset -4)
(setq c-brace-imaginary-offset 0)
(setq c-label-offset -4)
(setq comment-column 60)
(setq find-c-macro-definition-include-path '(t "." ".." "/usr/include"))
(setq macroexpand-ansi-c t)

;;
;; more nroff-mode customization
;;
(defun auto-pic-set-nroff-mode ()
  (nroff-mode)
  (electric-nroff-mode 1))

(defun auto-mm-set-nroff-mode ()
  (nroff-mode)
  (electric-nroff-mode 1)
  (auto-fill-mode 1))

(setq auto-mode-alist
  (append (list '("\\.mm$" . auto-mm-set-nroff-mode))
	  (list '("\\.pic$" . auto-pic-set-nroff-mode))
	  auto-mode-alist))

;;
;; show time and mail status
;;
(display-time)
