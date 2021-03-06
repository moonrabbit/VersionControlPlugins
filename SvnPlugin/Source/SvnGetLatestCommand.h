#pragma once

template<>
class SvnCommand<GetLatestRequest>
{
public:
    bool Run(SvnTask& task, GetLatestRequest& req, GetLatestResponse& resp)
    {
        std::string cmd = "update --accept=postpone ";
        cmd += Join(Paths(req.assets), " ", "\"");
        APOpen cppipe = task.RunCommand(cmd);

        std::string line;
        while (cppipe->ReadLine(line))
        {
            Enforce<SvnException>(!EndsWith(line, "is not a working copy"), "Project is not a subversion working copy.");
            req.conn.Log().Info() << line << "\n";
        }

        const bool recursive = false;
        task.GetStatus(req.assets, resp.assets, recursive);
            
        resp.Write();
        return true;
    }
};
