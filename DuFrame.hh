#ifndef DUFRAME_HH
#define DUFRAME_HH

#include <deque>

#include "wx/wx.h"
#include "wx/thread.h"
#include "wx/grid.h"
#include "wx/statusbr.h"

class DuFrame : public wxFrame, wxThreadHelper
{
public:
    DuFrame ();
    ~DuFrame () = default;

    void OnClose (wxCloseEvent & evt);
    void GotData (wxThreadEvent& evt);

    void StartThread (wxCommandEvent & evt);
    void StopThread (wxCommandEvent & evt);
private:
    bool FindNextBiggestFile ();
    void RelayBiggestFile ();
    wxThread::ExitCode Entry ();

    const char * topdir;

    struct Dir_and_size {
        Dir_and_size (const wxString & first, long second)
            : name (first),
              size (second),
              opened (false) {}
        wxString name;
        long size;
        bool opened;
    };

    std::deque<Dir_and_size> candidates;
    std::deque<Dir_and_size> sorted_candidates;
    wxCriticalSection dirdata_cs;
    wxTextCtrl * addressbar;
    wxGrid * grid;
    wxStatusBar * statusbar;
    wxDECLARE_EVENT_TABLE ();
};

#endif
