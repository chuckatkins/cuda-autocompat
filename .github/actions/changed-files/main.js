const core = require("@actions/core");
const yaml = require("js-yaml");
const { execSync } = require("child_process");
const { minimatch } = require("minimatch");
const fs = require("fs");

function gitShaExistsLocally(sha) {
  try {
    execSync(`git cat-file -e ${sha}^{commit}`, { stdio: 'ignore' });
    return true;
  } catch {
    return false;
  }
}

try {
    console.log(`GITHUB_EVENT_PATH="${process.env.GITHUB_EVENT_PATH}"`);
    const eventPath = process.env.GITHUB_EVENT_PATH;
    console.log(`GITHUB_EVENT_NAME="${process.env.GITHUB_EVENT_NAME}"`);
    const eventName = process.env.GITHUB_EVENT_NAME;
    const eventData = JSON.parse(fs.readFileSync(eventPath, "utf8"));
    console.log(`GITHUB_STEP_SUMMARY="${process.env.GITHUB_STEP_SUMMARY}"`);
    const summaryPath = process.env.GITHUB_STEP_SUMMARY;

    let base, head;
    if (eventName === "pull_request") {
        base = eventData.pull_request.base.sha;
        head = eventData.pull_request.head.sha;
    } else if (eventName === "push") {
        base = eventData.before;
        head = eventData.after;
    } else {
        base = "origin/main";
        head = "HEAD";
    }

    console.log("::group:: Git");

    if (gitShaExistsLocally(base)) {
        console.log(`Skipping fetch base=${base} (already exists)`);
    } else {
        console.log(`Fetching base=${base}`);
        execSync(`git fetch origin ${base} --depth=1`);
    }
    if (gitShaExistsLocally(head)) {
        console.log(`Skipping fetch head=${head} (already exists)`);
    } else {
        console.log(`Fetching head=${head}`);
        execSync(`git fetch origin ${head} --depth=1`);
    }

    console.log("Computing diff");
    const changedFiles = execSync(`git diff --name-only ${base} ${head}`, {
        encoding: "utf-8",
    })
        .split("\n")
        .filter(Boolean);

    console.log("::endgroup::");

    console.log("::group:: All changed files");
    core.setOutput("all", changedFiles.length.toString());
    if (changedFiles.length > 0) {
        console.log(`${changedFiles.length} file(s) changed`);
        for (const f of changedFiles) {
            console.log(f);
        }
    } else {
        console.log("No changed files");
    }
    if (summaryPath) {
        fs.appendFileSync(summaryPath, `## Changed Files Summary\n\n`);
        fs.appendFileSync(
            summaryPath,
            `**Total changed files:** ${changedFiles.length}\n\n`
        );
    }
    console.log("::endgroup::");

    console.log("::group::CI-related changes");
    const ciPatterns = [
        ".github/**",
        ".ci/**",
        ".github/workflows/**",
        "scripts/ci/**",
    ];
    const ciMatchedFiles = changedFiles.filter((file) =>
        ciPatterns.some((pattern) => minimatch(file, pattern))
    );

    core.setOutput("ci", ciMatchedFiles.length > 0 ? "true" : "false");

    if (ciMatchedFiles.length > 0) {
        console.log(`Changes detected - ci.should_run = true`);
        console.log(`${ciMatchedFiles.length} file(s) matched`);
        for (const f of changedFiles) {
            console.log(f);
        }
    } else {
        console.log("No matched files");
    }
    if (summaryPath) {
        fs.appendFileSync(
            summaryPath,
            `**CI-related files changed:** ${ciMatchedFiles.length}\n\n`
        );
    }
    console.log("::endgroup::");

    const filtersYaml = core.getInput("filters", { required: true });
    const filters = yaml.load(filtersYaml);

    for (const [group, patterns] of Object.entries(filters)) {
        console.log(`::group::Filter group: ${group}`);
        const matchedFiles = changedFiles.filter((file) =>
            patterns.some((pattern) => minimatch(file, pattern))
        );

        const count = matchedFiles.length;
        const shouldRun = ciMatchedFiles.length > 0 || count > 0;

        core.setOutput(group, shouldRun ? "true" : "false");

        if (shouldRun && ciMatchedFiles.length > 0) {
            console.log("should_run = true triggered by CI changes");
        }
        if (count > 0) {
            console.log(`${count} file(s) matched`);
            for (const f of matchedFiles) {
                console.log(f);
            }
        } else {
            console.log(`No matched files`);
        }

        if (summaryPath) {
            fs.appendFileSync(summaryPath, `### Group ${group}\n`);
            fs.appendFileSync(summaryPath, `- Matched files: ${count}\n`);
            fs.appendFileSync(summaryPath, `- should_run: ${shouldRun}\n`);
            if (count > 0) {
                fs.appendFileSync(
                    summaryPath,
                    matchedFiles.map((f) => `  - ${f}\n`).join("")
                );
            }
            fs.appendFileSync(summaryPath, `\n`);
        }
        console.log(`::endgroup::`);
    }
} catch (err) {
    core.setFailed(err.message);
}
