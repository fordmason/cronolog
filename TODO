* Rewrite cronolog as an Apache module as well as a standalone program.

* Option to run an extenal program (such as wwwstat or gzip) on each
  log file once it is finished with.
  (Problem is: if multiple processes are logging to the same file, how
  do we know which one should run the program?)

* Test suite (this should be straightforward with new debug options).

* Option to specify the rotation period explicitly.  The template might
  imply a more frequent rotation period, but would have to be able to
  differentiate the different log files.  For example you could say
  rotate hourly but include a minute specifier in the template, but it
  would be illegal to say rotate hourly but have a template of
  "%Y%m%d" (which implies rotate daily).
