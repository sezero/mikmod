/*

 Mikmod Sound System - The Legend Continues

  By Jake Stine and Hour 13 Studios (1996-2002)

 Support:
  If you find problems with this code, send mail to:
    air@hour13.com
  For additional information and updates, see our website:
    http://www.hour13.com

 ---------------------------------------------------
 load_ogg.c

 The Ogg Vorbis compressed audio format loader!  Loads an ogg sample and converts
 it into a SAMPLE (decompressed).  To be honest, you shouldn't have to or want to
 use this loader much, because by loading into SAMPLE handles implies you intend
 to manipulate and re-save the given data.  And really, you shouldn't compress
 to Vorbis until after all manipulations are complete.

*/
