#pragma once
#include "SvnTask.h"
#include <fstream>

// Submit == Svn commit
template<>
class SvnCommand<SubmitRequest>
{
public:
    bool Run(SvnTask& task, SubmitRequest& req, SubmitResponse& resp)
    {
        bool saveOnly = req.args.size() > 1 && req.args[1] == "saveOnly";
        const bool recursive = false;


        // saveOnly option really only makes sense for perforce but is 
        // also used to move assets to a newly constructed changelist
        if (saveOnly) 
        {
            req.conn.Progress(-1, 0, "Saving changelist");
            std::string createChangelistCmd = "changelist ";
            std::string firstLineOfDescription = req.changelist.GetDescription();
            firstLineOfDescription = firstLineOfDescription.substr(0, firstLineOfDescription.find('\n'));
            createChangelistCmd += "\"";
            createChangelistCmd += firstLineOfDescription;
            //createChangelistCmd += req.changelist.GetRevision() == kNewListRevision ? 
            //    firstLineOfDescription : req.changelist.GetRevision();
            createChangelistCmd += "\"";
            createChangelistCmd += " ";
            createChangelistCmd += Join(Paths(req.assets), " ", "\"");
            APOpen cppipe = task.RunCommand(createChangelistCmd);

            std::string line;
            while (cppipe->ReadLine(line))
            {
                Enforce<SvnException>(!EndsWith(line, "is not a working copy"), "Project is not a subversion working copy.");
                req.conn.Log().Info() << line << "\n";
            }
            
            task.GetStatus(req.assets, resp.assets, recursive);
            
            resp.Write();
            return true;
        }

        std::string tmpfilepath = task.GetProjectPath() + "/Temp/svncommitmessage.txt";
        {
            std::ofstream os(tmpfilepath.c_str());
            os << req.changelist.GetDescription();
            os.flush();
        }
        
        std::string cmd = "commit --depth=empty -F \"";
        cmd += tmpfilepath;
        cmd += "\" ";
        if (req.changelist.GetRevision() != kNewListRevision &&
            req.changelist.GetRevision() != kDefaultListRevision)
        {
            cmd += "--cl \"";
            cmd += req.changelist.GetRevision();
            cmd += "\" ";
        }
        cmd += Join(Paths(req.assets), " ", "\"");
        
        req.conn.Progress(-1, 0, "submitting");
        APOpen ppipe = task.RunCommand(cmd);
        
        std::string line;
        while (ppipe->ReadLine(line))
        {
            Enforce<SvnException>(!EndsWith(line, "is not a working copy"), "Project is not a subversion working copy.");
            req.conn.Log().Info() << line << "\n";
            
            if (line.find("is not under version control and is not part of the commit, yet its child") != std::string::npos)
                req.conn.WarnLine(line.substr(5)); // strip "svn: "
        }
        
        task.GetStatus(req.assets, resp.assets, recursive);

        resp.Write();
        return true;
    }
};
