#include "TH1.h"
#include "TCanvas.h"
#include "TROOT.h"
#include "TFile.h"

int main()
{
    gROOT->SetStyle("Plain");

    TFile *f = new TFile("pedestalHistograms_3-8.root");

    TH1D *hped[192];

    TH1D *peds = new TH1D("peds", "pedestals", 20, 0, 10);
    TH1D *rmss = new TH1D("rmss", "pedestal rms", 10, 0, 1);

    bool first = true;
    TCancas c3("c3", "c3", 800, 800);

    for(int i = 0; i < 36; i++)
    {
        c2->cd();
        char hname[128];
        sprintf(hname, "h%d", i + 18);
        hped[i] = (TH1D*)f->Get(hname);

        hped[i]->GetXaxis()->SetTitle("ADC counts");
        hped[i]->GetXaxis()->SetRangeUser(0, 40);
        hped[i]->SetTitle(0);

        if(first)
        {
            hped[i]->Draw();
            first = false;
        }
        else hped[i]->Draw("same");

        TCanvas c1("c1","c1", 800, 800);
        c1->cd();
        hped[i]->Draw();

        char pname[128];
        sprintf(pname, "pedestalDist_Chan_%d.png", i + 1);
        c1.Print(pname);

        peds->Fill(hped[i]->GetMean());
        rmss->Fill(hped[i]->GetRMS());
    }

    TCanvas c1("c1","c1", 800, 800);
    peds->GetXaxis()->SetTitle("Mean ADC counts");
    peds->SetTitle(0);
    //peds->SetStats(0);
    peds->Draw();
    c1.Print("pedestalDist.png");

    TCanvas c2("c2","c2", 800, 800);
    rmss->GetXaxis()->SetTitle("RMS ADC counts");
    rmss->SetTitle(0);
    //rmss->SetStats(0);
    rmss->Draw();
    c2.Print("pedestalRMSDist.png");
}
