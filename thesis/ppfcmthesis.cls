%
% This is a LaTeX class for master and bachelor theses following the guidelines
% of Faculty of Computer Science at Poznan University of Technology.
%
% This class is UNOFFICIAL and NOT ENDORSED by the university.
%
% (c) 2006--2007 Dawid Weiss
% Poznan University of Technology
%
%
\NeedsTeXFormat{LaTeX2e}[1995/12/01]
\ProvidesClass{ppfcmthesis}[2007/02/18 v0.94 PP FCM thesis class]

% v.0.94 Added "bachelor" option for bachelor theses. 
% v.0.93 Varioref is loaded prior to hyperref.

% Declare bachelor thesis switch.
\newif\if@ppbachelor\@ppbachelorfalse

%
% Declare default options and load base package (memoir).
%
\DeclareOption{bachelor}
   {\@ppbachelortrue}

\ExecuteOptions{a4paper,pl,11pt,oneside,final}
\DeclareOption*{\PassOptionsToClass{\CurrentOption}{memoir}}
\ProcessOptions\relax
\LoadClass{memoir}

%
% Require additional packages
%
\RequirePackage{babel}
\newif\if@pppolish
\iflanguage{polish}{\@pppolishtrue}{\@pppolishfalse}

\selectlanguage{english}

\RequirePackage{setspace}
\RequirePackage{booktabs}
\RequirePackage{graphicx}

\ifpdf
	\DeclareGraphicsExtensions{.pdf,.png,.jpg}
\else
	\DeclareGraphicsExtensions{.eps}
\fi
\RequirePackage{fancyvrb}

%
% Configure Hyperref
%

\RequirePackage{url}
\RequirePackage{nameref}
\RequirePackage{varioref}
  
\ifpdf
        \usepackage[unicode,linktocpage,bookmarks]{hyperref}
\else
        \usepackage[dvips,unicode,linktocpage,bookmarks]{hyperref}
\fi
\hypersetup{
    pdffitwindow=true,
    bookmarksnumbered=true,
    bookmarksopen=true,
    colorlinks=true,
    pdfpagelayout=SinglePage,
    pdfpagemode=UseOutlines,
    pdfstartview=Fit,
    linkcolor=black,
    citecolor=black,
    anchorcolor=black,
    filecolor=black,
    menucolor=black,
    urlcolor=black,
    pdftitle={},
    pdfauthor={},
    pdfkeywords={}
}
\RequirePackage{memhfixc}

%
% Customize title page
%
\aliaspagestyle{title}{empty}
\date{}
\newcommand{\ppsupervisor}[1]{\def\@ppsupervisor{#1}}
\newcommand{\ppyear}[1]{\def\@ppyear{#1}}

\setlength{\droptitle}{0cm}

\renewcommand{\maketitlehooka}{%
\begin{center}
Poznan University of Technology\\Faculty of Computing Science\\Institute of Computing Science
\end{center}}

\pretitle{\vspace{2cm}\begin{center}%
\if@pppolish%
\if@ppbachelor Praca dyplomowa in{\.z}ynierska\else Praca dyplomowa magisterska\fi
\else
\if@ppbachelor Bachelor's thesis\else Master's thesis\fi
\fi\par\vspace{1cm}\Large\bfseries\MakeUppercase}
\posttitle{\end{center}}

\newcommand{\album}[1]{\unskip ,~#1}
\renewcommand{\and}{\\}

\renewcommand{\maketitlehookb}{\vspace{1cm}}

\renewcommand{\maketitlehookc}{\vfill\begin{center}
Supervisors\\\@ppsupervisor
\end{center}
\vspace{2cm}\begin{center}Pozna\'n, \@ppyear\if@pppolish~r.\fi\end{center}
}

%
% Define colophon 
%
\newcommand{\ppcolophon}{%
\renewcommand{\and}{\unskip{}, }%
\renewcommand{\album}[1]{\unskip}%
\cleardoublepage\thispagestyle{empty}\vspace*{\fill}%
\footnotesize\singlespacing\noindent%
\raisebox{-1.3cm}{\includegraphics[height=1.5cm]{figures/template/logo-pp}}%
\hspace{1.5em}%
\begin{minipage}[t]{11cm}%
\vglue 0pt\copyright{} \@ppyear\ \@author

\medskip\noindent%
Poznan University of Technology\\Faculty of Computing Science\\Institute of Computing Science

\medskip\noindent%
% \if@pppolish%
% Sk{\l}ad przy u{\.z}yciu systemu \LaTeX{}.
% \else%
% Typeset using \LaTeX\ in Computer Modern.
% \fi

\renewcommand{\and}{\unskip{} $\backslash$and }%
% \medskip\noindent Bib\TeX{}:\\[1ex]
% \tiny\texttt{%
% @mastersthesis\{ key,\\
% \hspace*{2em}author = "\@author",\\
% \hspace*{2em}title = "\{\@title\}",\\
% \hspace*{2em}school = "Poznan University of Technology",\\
% \hspace*{2em}address = "Pozna\{$\backslash$'n\}, Poland",\\
% \hspace*{2em}year = "\@ppyear",\\
% \}
% }
\end{minipage}
}

%
% Customize running titles and page style.
%

\makepagestyle{ppfcmthesis}
\makeevenfoot{ppfcmthesis}{}{}{}
\makeatletter
\newcommand{\@ppfcmthesismarks}{%
  \let\@mkboth\markboth
  \def\chaptermark##1{\markboth{##1}{##1}}    % left mark & right marks
  \def\sectionmark##1{\markright{%
    \ifnum \c@secnumdepth>\z@
      \thesection. \ %
    \fi
    ##1}}
  \def\tocmark{\markboth{\contentsname}{}}
  \def\lofmark{\markboth{\listfigurename}{}}
  \def\lotmark{\markboth{\listtablename}{}}
  \def\bibmark{\markboth{\bibname}{}}
  \def\indexmark{\markboth{\indexname}{}}
}
\makepsmarks{ppfcmthesis}{\@ppfcmthesismarks}
\makeatother
\makeevenhead{ppfcmthesis}{\thepage}{}{\normalfont\small\itshape\leftmark}
\makeoddhead{ppfcmthesis}{\normalfont\small\itshape\rightmark}{}{\thepage}

%
% Customize Table of Contents
%
\setcounter{tocdepth}{3}
\maxtocdepth{subsubsection}
\maxsecnumdepth{subsection}

%
% Customize book style
%
% \tightlists
\pagestyle{ppfcmthesis}
\setstocksize{297mm}{210mm}
\settrimmedsize{297mm}{210mm}{*}
\settrims{0pt}{0cm}
\setlrmarginsandblock{3.5cm}{2.5cm}{*}
\setulmarginsandblock{2.5cm}{2.5cm}{*}
\setmarginnotes{6pt}{50pt}{\onelineskip}
\setheadfoot{\onelineskip}{2\onelineskip}
\setheaderspaces{*}{2\onelineskip}{*}

\changecaptionwidth
\captionwidth{.9\linewidth}
\captionnamefont{\footnotesize\scshape}
\captiontitlefont{\footnotesize}

\newcommand{\fcmfcaption}[2][\@empty]{
\captionnamefont{\footnotesize\bfseries}%
\captiondelim{.~}
\captionstyle{}
\captiontitlefont{\footnotesize}
\ifx #1\@empty \caption{#2}\else \caption[#1]{#2}}

\newcommand{\fcmtcaption}[2][\@empty]{
\captionnamefont{\footnotesize\bfseries\hfill}%
\captiondelim{}
\captionstyle{\raggedleft\\}
\captiontitlefont{\footnotesize\normalfont}
\ifx #1\@empty \caption{#2}\else \caption[#1]{#2}}

\def\thesubsubsection {\thesubsection .\arabic{subsubsection}}

\checkandfixthelayout