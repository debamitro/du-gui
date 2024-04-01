
#include "wx/wx.h"

#include "DuApp.hh"
#include "DuFrame.hh"

bool DuApp::OnInit ()
{
    if (!wxApp::OnInit())
    {
        return false;
    }

    auto frame = new DuFrame(argc > 1 ? argv[1] : "");

    frame->Show (true);

    return true;
}
