#include "wx/wx.h"
#include "wx/grid.h"
#include "wx/dir.h"
#include <sys/stat.h>
#include <iostream>
#include <algorithm>

#include "DuFrame.hh"

enum
{
    GOT_DATA = 500,
    DONE_SEARCHING,
    START_THREAD,
    STOP_THREAD
};

wxBEGIN_EVENT_TABLE (DuFrame, wxFrame)
    EVT_THREAD (GOT_DATA, DuFrame::GotData)
    EVT_THREAD (DONE_SEARCHING, DuFrame::DoneSearching)
    EVT_BUTTON (START_THREAD, DuFrame::StartThread)
    EVT_BUTTON (STOP_THREAD, DuFrame::StopThread)
wxEND_EVENT_TABLE ()


DuFrame::DuFrame ()
    : wxFrame (nullptr, wxID_ANY, "Disk Usage"),
      wxThreadHelper (),
      topdir (nullptr),
      candidates (),
      sorted_candidates (),
      dirdata_cs (),
      addressbar (nullptr),
      grid (nullptr),
      statusbar (nullptr)
{
    auto sizer = new wxBoxSizer (wxVERTICAL);

    auto sizer2 = new wxBoxSizer (wxHORIZONTAL);
    sizer->Add (sizer2, wxSizerFlags().Expand());
    addressbar = new wxTextCtrl (this, wxID_ANY, topdir);
    sizer2->Add (addressbar, wxEXPAND);

    auto gobutton = new wxButton (this, START_THREAD, "go");
    sizer2->Add (gobutton, wxEXPAND);

    auto stopbutton = new wxButton (this, STOP_THREAD, "stop");
    sizer2->Add (stopbutton, wxEXPAND);

    grid = new wxGrid (this, wxID_ANY);
    grid->CreateGrid (2, 2);
    grid->SetColLabelValue (0, "Directory");
    grid->SetColLabelValue (1, "Size");
    sizer->Add (grid, wxSizerFlags().Expand());
    statusbar = new wxStatusBar (this, wxID_ANY);
    statusbar->SetFieldsCount (2);
    SetStatusBar (statusbar);
    SetSizerAndFit (sizer);
}

void DuFrame::StartThread(wxCommandEvent & evt)
{
    topdir = strdup(addressbar->GetValue().c_str());
    candidates.clear ();
    sorted_candidates.clear ();
    grid->ClearGrid ();

    if (CreateThread(wxTHREAD_JOINABLE) != wxTHREAD_NO_ERROR)
    {
        wxLogError("Could not create the worker thread!");
        return;
    }

    candidates.emplace_back (Dir_and_size (topdir, 0));
    if (GetThread()->Run() != wxTHREAD_NO_ERROR)
    {
        wxLogError("Could not run the worker thread!");
        return;
    }
    SetStatusText("Searching");
}

void DuFrame::StopThread (wxCommandEvent & evt)
{
    if (GetThread() && GetThread()->IsRunning())
    {
        GetThread()->Delete();
        SetStatusText ("Stopped");
    }
}

void DuFrame::OnClose (wxCloseEvent & evt)
{
    if (GetThread() && GetThread()->IsRunning())
    {
        GetThread()->Wait();
    }

    Destroy();
}

wxThread::ExitCode DuFrame::Entry ()
{
    while (FindNextBiggestFile ())
    {
        if (GetThread()->TestDestroy())
        {
            return 0;
        }

        {
            wxCriticalSectionLocker lock(dirdata_cs);
            RelayBiggestFile ();
        }

        wxQueueEvent(GetEventHandler(), new wxThreadEvent(wxEVT_THREAD, GOT_DATA));
        wxMilliSleep (100);
    }

    wxQueueEvent(GetEventHandler(), new wxThreadEvent(wxEVT_THREAD, DONE_SEARCHING));
    return 0;
}

bool DuFrame::FindNextBiggestFile ()
{
    auto one_file_or_dir = candidates.begin ();

    for (; one_file_or_dir != candidates.end(); ++one_file_or_dir)
    {
        if (!one_file_or_dir->opened)
        {
            break;
        }
    }

    if (one_file_or_dir == candidates.end())
    {
        return false;
    }

    one_file_or_dir->opened = true;
    wxDir dir(one_file_or_dir->name);

    if (dir.IsOpened ())
    {
        wxString filename;
        bool cont = dir.GetFirst (&filename);
        while (cont)
        {
            struct stat st;
            wxString fullpath = one_file_or_dir->name;
            fullpath += "/";
            fullpath += filename;
            if (stat (fullpath.c_str (), &st) == 0)
            {
                if ((st.st_mode & S_IFDIR) != 0)
                {
                    candidates.emplace_back (Dir_and_size (fullpath, 0));
                }
                else if ((st.st_mode & S_IFREG) != 0)
                {
                    one_file_or_dir->size += st.st_size;
                }
            }

            cont = dir.GetNext (&filename);
        }
    }

    return true;
}

void DuFrame::RelayBiggestFile ()
{
    sorted_candidates = candidates;
    std::sort (sorted_candidates.begin (), sorted_candidates.end(),
               [](Dir_and_size a, Dir_and_size b)
    {
        return a.size > b.size;
    });
}

void DuFrame::GotData (wxThreadEvent& evt)
{
    std::string commentary = "Processed ";
    commentary += std::to_string (candidates.size());
    commentary += " files";
    SetStatusText (commentary, 1);
    grid->ClearGrid();

    wxCriticalSectionLocker lock (dirdata_cs);
    sorted_candidates.erase(std::remove_if (sorted_candidates.begin(),
                                            sorted_candidates.end(),
    [](const Dir_and_size & dirsize) {
        return dirsize.size == 0;
    }),
    sorted_candidates.end());
    if (grid->GetNumberRows () < sorted_candidates.size ())
    {
        grid->InsertRows (0, sorted_candidates.size() - grid->GetNumberRows ());
    }
    int i = 0;
    for (auto dir_and_size : sorted_candidates)
    {
        grid->SetCellValue (i, 0, dir_and_size.name);
        grid->SetCellValue (i, 1, FormattedFileSize(dir_and_size.size));
        ++i;
    }
}

void DuFrame::DoneSearching (wxThreadEvent & evt)
{
    SetStatusText ("Done");
}

std::string DuFrame::FormattedFileSize (size_t sz)
{
    const char * units[4] = {"B","KB","MB","GB"};
    size_t readable = sz;
    int i = 0;

    if (readable >= 1024)
    {
        do
        {
            readable /= 1024;
            ++i;
        } while (readable > 1024);
    }

    std::string s = std::to_string(readable);
    s += units[i];
    return s;
}
