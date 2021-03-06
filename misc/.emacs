; start package.el with emacs
(require 'package)

; add MELPA to repository list
(add-to-list 'package-archives '("melpa" . "http://melpa.milkbox.net/packages/"))
(package-initialize)

; determine the underlying operating system
(setq casey-aquamacs (featurep 'aquamacs))
(setq casey-linux (featurep 'x))
(setq casey-win32 (not (or casey-aquamacs casey-linux)))

; set up recently opened files list
(require 'recentf)
(recentf-mode 1)
(setq recentf-max-menu-items 25)
(global-set-key "\C-x\ \C-r" 'recentf-open-files)

; start auto-complete
(require 'auto-complete)
(require 'auto-complete-config)
(ac-config-default)

; start auto-complete for C/C++ headers
(defun my:ac-c-header-init ()
  (require 'auto-complete-c-headers)
  (add-to-list 'ac-sources 'ac-source-c-headers)
  (add-to-list 'achead:include-directories '"c:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include")
  )
(add-hook 'c++-mode-hook 'my:ac-c-header-init)
(add-hook 'c-mode-hook 'my:ac-c-header-init)

; start buffer selection
(global-set-key (kbd "C-x C-b") 'bs-show)

; start neotree
(add-to-list 'load-path "C:/work")
(require 'neotree)
(global-set-key [f8] 'neotree-toggle)
(setq neo-smart-open t)

; start yasnippet
(require 'yasnippet)
(yas-global-mode 1)

; start iedit
(define-key global-map (kbd "C-;") 'iedit-mode)

; style
(setq c-default-style "bsd"
      c-basic-offset 4)
(c-set-offset 'case-label 4)

; set default mode for .h
(add-to-list 'auto-mode-alist '("\\.h\\'" . c++-mode))

; big c hook
(defun graham-big-c-hook ()
  ; auto-comment files
  (defun graham-header-format ()
    "Format the given file as a header file."
    (interactive)
    (setq BaseFileName (file-name-sans-extension (file-name-nondirectory buffer-file-name)))
    (insert "#pragma once")
    (insert "\n/* ========================================================================\n")
    (insert (concat "   File: " BaseFileName "\n"))
    (insert "   Date: \n")
    (insert "   Revision: \n")
    (insert "   Creator: Graham Zuber \n")
    (insert "   Notice: (C) Copyright 2015 by Graham Zuber All Rights Reserved.\n")
    (insert "   ======================================================================== */\n\n")
    )

  (defun graham-source-format ()
    "Format the given file as a source file."
    (interactive)
    (setq BaseFileName (file-name-sans-extension (file-name-nondirectory buffer-file-name)))
    (insert "/* ========================================================================\n")
    (insert (concat "   File: " BaseFileName "\n"))
    (insert "   Date: \n")
    (insert "   Revision: \n")
    (insert "   Creator: Graham Zuber \n")
    (insert "   Notice: (C) Copyright 2015 by Graham Zuber All Rights Reserved.\n")
    (insert "   ======================================================================== */\n\n")
    )

  (cond ((file-exists-p buffer-file-name) t)
	((string-match "[.]hin" buffer-file-name) (graham-source-format))
	((string-match "[.]cin" buffer-file-name) (graham-source-format))
	((string-match "[.]h" buffer-file-name) (graham-header-format))
	((string-match "[.]cpp" buffer-file-name) (graham-source-format)))

  ; find corresponding file
  (defun casey-find-corresponding-file ()
    "Find the file that corresponds to this one."
    (interactive)
    (setq CorrespondingFileName nil)
    (setq BaseFileName (file-name-sans-extension buffer-file-name))
    (if (string-match "\\.c" buffer-file-name)
	(setq CorrespondingFileName (concat BaseFileName ".h")))
    (if (string-match "\\.h" buffer-file-name)
	(if (file-exists-p (concat BaseFileName ".c")) (setq CorrespondingFileName (concat BaseFileName ".c"))
	  (setq CorrespondingFileName (concat BaseFileName ".cpp"))))
    (if (string-match "\\.hin" buffer-file-name)
	(setq CorrespondingFileName (concat BaseFileName ".cin")))
    (if (string-match "\\.cin" buffer-file-name)
	(setq CorrespondingFileName (concat BaseFileName ".hin")))
    (if (string-match "\\.cpp" buffer-file-name)
	(setq CorrespondingFileName (concat BaseFileName ".h")))
    (if CorrespondingFileName (find-file CorrespondingFileName)
      (error "Unable to find a corresponding file")))
  (defun casey-find-corresponding-file-other-window ()
    "Find the file that corresponds to this one."
    (interactive)
    (find-file-other-window buffer-file-name)
    (casey-find-corresponding-file)
    (other-window -1))
  (define-key c++-mode-map [f12] 'casey-find-corresponding-file)
  (define-key c++-mode-map [M-f12] 'casey-find-corresponding-file-other-window)

  ; error parsing
  (add-to-list 'compilation-error-regexp-alist 'casey-devenv)
  (add-to-list 'compilation-error-regexp-alist-alist '(casey-devenv "*\\([0-9]+>\\)?\\(\\(?:[a-zA-Z]:\\)?[^:(\t\n]+\\)(\\([0-9]+\\)) : \\(?:see declaration\\|\\(?:warnin\\(g\\)\\|[a-z ]+\\) C[0-9]+:\\)" 2 3 nil (4)))
  )
(add-hook 'c-mode-common-hook 'graham-big-c-hook)

(custom-set-variables
 ;; custom-set-variables was added by Custom.
 ;; If you edit it by hand, you could mess it up, so be careful.
 ;; Your init file should contain only one such instance.
 ;; If there is more than one, they won't work right.
 '(ansi-color-faces-vector
   [default default default italic underline success warning error])
 '(custom-enabled-themes (quote (wombat))))
(custom-set-faces
 ;; custom-set-faces was added by Custom.
 ;; If you edit it by hand, you could mess it up, so be careful.
 ;; Your init file should contain only one such instance.
 ;; If there is more than one, they won't work right.
 )

; emacs style options
(global-linum-mode t)
(add-to-list 'default-frame-alist '(font . "Source Code Pro-12"))
(set-face-attribute 'default t :font "Source Code Pro-12")
(setq split-width-threshold 1)

; hightlight TODOs
(setq fixme-modes '(c++-mode c-mode emacs-lisp-mode))
 (make-face 'font-lock-fixme-face)
 (make-face 'font-lock-study-face)
 (make-face 'font-lock-important-face)
 (make-face 'font-lock-note-face)
 (mapc (lambda (mode)
	 (font-lock-add-keywords
	  mode
	  '(("\\<\\(TODO\\)" 1 'font-lock-fixme-face t)
	    ("\\<\\(STUDY\\)" 1 'font-lock-study-face t)
	    ("\\<\\(IMPORTANT\\)" 1 'font-lock-important-face t)
            ("\\<\\(NOTE\\)" 1 'font-lock-note-face t))))
	fixme-modes)
 (modify-face 'font-lock-fixme-face "Red" nil nil t nil t nil nil)
 (modify-face 'font-lock-study-face "Yellow" nil nil t nil t nil nil)
 (modify-face 'font-lock-important-face "Yellow" nil nil t nil t nil nil)
 (modify-face 'font-lock-note-face "Dark Green" nil nil t nil t nil nil)


; make code
(setq compilation-directory-locked nil)
(when casey-win32
  (setq casey-makescript "build.bat")
  )
(setq compilation-context-lines 0)
(setq compilation-error-regexp-alist
    (cons '("^\\([0-9]+>\\)?\\(\\(?:[a-zA-Z]:\\)?[^:(\t\n]+\\)(\\([0-9]+\\)) : \\(?:fatal error\\|warnin\\(g\\)\\) C[0-9]+:" 2 3 nil (4))
     compilation-error-regexp-alist))

(defun find-project-directory-recursive ()
  "Recursively search for a makefile."
  (interactive)
  (if (file-exists-p casey-makescript) t
      (cd "../")
      (find-project-directory-recursive)))

(defun lock-compilation-directory ()
  "The compilation process should NOT hunt for a makefile"
  (interactive)
  (setq compilation-directory-locked t)
  (message "Compilation directory is locked."))

(defun unlock-compilation-directory ()
  "The compilation process SHOULD hunt for a makefile"
  (interactive)
  (setq compilation-directory-locked nil)
  (message "Compilation directory is roaming."))

(defun find-project-directory ()
  "Find the project directory."
  (interactive)
  (setq find-project-from-directory default-directory)
  (switch-to-buffer-other-window "*compilation*")
  (if compilation-directory-locked (cd last-compilation-directory)
  (cd find-project-from-directory)
  (find-project-directory-recursive)
  (setq last-compilation-directory default-directory)))

(defun make-without-asking ()
  "Make the current build."
  (interactive)
  (if (find-project-directory) (compile casey-makescript))
  (other-window 1))
(define-key global-map [f11] 'make-without-asking)
