\ProvidesClass{scidoc}
\LoadClass{report}
\RequirePackage{xspace}
\RequirePackage{verbatim}
\newcommand{\method}[1]{\textbf{(\theclass #1)}\par}
\newcommand{\callsynt}[1]{\medskip\noindent\textbf{(\theclass #1)}\nopagebreak\par\medskip}
\newcommand{\methname}[1]{\textbf{#1}}
\newcommand{\parameter}[1]{\emph{#1}}
\newcommand{\classname}[1]{\mbox{\underline{#1}}}
\newcommand\sectclass{\@startsection{subsubsection}{3}{\z@}%
                                     {-3.25ex\@plus -1ex \@minus -.2ex}%
                                     {1.5ex \@plus .2ex}%
                                     {\normalfont\normalsize\bfseries\itshape}}
\newcommand{\superclass}[1]{\makebox[2in][l]{\textbf{Superclass:}}\textbf{#1}\par}
\let\theclass=\relax
\newenvironment{class}[1]{\def\theclass{#1\xspace}\clearpage\subsection{#1 class}%
}{}
\newcommand{\returns}[1]{\par Returns:\qquad#1}
\newenvironment{class*}[1]{\def\theclass{#1\xspace}\subsection{#1 class}%
}{}
\newenvironment{methods}{\ifx\theclass{\relax}%
\errmessage{methods environment used outside of class}%
\else\sectclass*{Methods of the \theclass class:}\par%
\fi}{\par}
\newenvironment{properties}{\ifx\theclass{\relax}%
\errmessage{properties environment used outside of class}%
\else\sectclass*{Properties of the \theclass class:}\par%
\fi}{\par}
\newcommand{\subfxname}[1]{\textbf{#1}}
\newcommand{\kcallname}[1]{\textbf{#1}}
\newenvironment{kernelcall}[1]{\def\theclass{#1\xspace}\clearpage\subsection{#1 syscall}%
}{}
\newenvironment{kernelcall*}[1]{\def\theclass{#1\xspace}\subsection{#1 syscall}%
}{}
\newenvironment{subfx}{\ifx\theclass{\relax}%
\errmessage{subfx environment used outside of kernelcall}%
\else\sectclass*{Subfunctions of the \theclass syscall:}\par%
\fi}{\par}
