#ifndef DUAPP_HH
#define DUAPP_HH

#include "wx/wx.h"

class DuApp : public wxApp
{
public:
  DuApp () = default;
  ~DuApp () = default;
  bool OnInit ();
};

#endif
