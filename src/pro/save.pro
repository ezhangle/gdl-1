;+
; NAME: SAVE
;
; PURPOSE:
;       Serves as a wrapper around CMSAVE from Craig B. Markwardt CMVSLIB
;       library. You must download and install yourself this CMVSLIB library
;       then add it PATH in your GDL_PATH. This library can be found here:
;       http://cow.physics.wisc.edu/~craigm/idl/cmsave.html
;
; MODIFICATION HISTORY:
;   01-Sep-2006 : written by Joel Gales
;   15-dec-2006 : modifications by Alain Coulais (AC)
;     1/ explicite HTTP link in header to external CMVSLIB library 
;     2/ test via EXECUTE() in pro to warn new users for missing
;           external CMVSLIB library  
;     3/ final test if we need to do or not since CMSAVE crash if nothing to do
;   13-feb-2007 : modifications by AC
;     1/ better management of void call to CMSAVE (case wich give a crash)
;     2/ explicit test and message if unamed variables
;     3/ explicit warning if /all keyword (not managed by CMSAVE)
;     4/ if no filename, indicate the name of the default filename
;
; LICENCE:
; Copyright (C) 2006, J. Gales
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
;-

pro save,    p0,  p1,  p2,  p3,  p4,  p5,  p6,  p7,  p8,  p9, $
             p10, p11, p12, p13, p14, p15, p16, p17, p18, p19, $
             p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, $
             filename=filename0, verbose=verbose, xdr=xdr, $
             compatible=compat0, append=append, all=all, $
             status=status, varstatus=colstatus, mtimes=mtimes, $
             names=names, data=data, pass_method=method, $
             errmsg=errmsg, quiet=quiet, nocatch=nocatch, useunit=useunit, $
             test=test
;
ON_ERROR, 2

; is the external CMSVlib present ?
if (EXECUTE('res=CMSVLIB(/QUERY)') EQ 0) then begin
   print, "% SAVE: Missing CMSVlib in your IDL PATH"
   print, "% SAVE: please read SAVE header for help."
   return
endif

; Positional Parameters
parm = 'cmsave,'
for __i__=0,n_params()-1 do begin
   p = strcompress('p' + string(__i__), /remove_all)
   stat = execute('arg=routine_names(' + p +',arg_name=-1)')
   ;; we manage "arg" to reject "unmaned" variables
   if (STRLEN(arg) LT 1) then begin
      message="Expression must be named variable in this context: <"
      message=message+STRCOMPRESS(HELPFORM('', p1,/short)+">.")
      ;; AC 27/02/2007: will be better to not do a /continue
      ;; but currently hard to escape from the ON_ERROR, 2 in GDL
      MESSAGE, message,/continue
      return
   endif
   cmd = arg + '=temporary(' + p + ')'
   cmd = arg + '=' + p
   stat = execute(cmd[0])
   parm = parm + arg + ','
endfor

parm = strmid(parm, 0, strlen(parm)-1)

;AC 15/12/06: when calling without argument, CMSAVE gives a
;segmentation fault --> better way to stop
;
if (STRCOMPRESS(STRUPCASE(parm),/remove_all) EQ 'CMSAVE') then begin
   MESSAGE, "no VAR. provided, nothing to do", /continue
   return
endif
;
; Keywords
;
; special case for beginners: what is the default name of the filename ?!
;
if (n_elements(filename0) NE 0) then begin
   parm = parm + ',filename=filename0'
endif else begin
   filename_def = 'cmsave.sav'  ; if future change, see inside CMSAVE 
   print, "% SAVE: default FILENAME is used : ", filename_def
endelse
;
if (n_elements(verbose) ne 0) then parm = parm + ',verbose=verbose'
if (n_elements(xdr) ne 0) then parm = parm + ',xdr=xdr'
if (n_elements(compat0) ne 0) then parm = parm + ',compatible=compat0'
if (n_elements(append) ne 0) then parm = parm + ',append=append'
if (n_elements(status) ne 0) then parm = parm + ',status=status'
if (n_elements(colstatus) ne 0) then parm = parm + ',varstatus=colstatus'
if (n_elements(mtimes) ne 0) then parm = parm + ',mtimes=mtimes'
if (n_elements(names) ne 0) then parm = parm + ',names=names'
if (n_elements(data) ne 0) then parm = parm + ',data=data'
if (n_elements(method) ne 0) then parm = parm + ',pass_method=method'
if (n_elements(errmsg) ne 0) then parm = parm + ',errmsg=errmsg'
if (n_elements(quiet) ne 0) then parm = parm + ',quiet=quiet'
if (n_elements(nocatch) ne 0) then parm = parm + ',nocatch=nocatch'
if (n_elements(useunit) ne 0) then parm = parm + ',useunit=useunit'
;
; special case: This important keyword is not working now.
;
if (n_elements(all) ne 0) then begin
   parm = parm + ',all=all'
   MESSAGE, " warning : keyword /ALL not working now", /continue
endif
;
; help for debugging ...
;
if KEYWORD_SET(test) then begin
   print, parm
   STOP
endif
;
; real call to the CMSAVE routine
;
stat=execute(parm[0])
;
return
end
