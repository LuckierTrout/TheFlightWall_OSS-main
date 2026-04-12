# Acceptance Auditor Prompt

Review target: `fn-1-4-ota-self-check-and-rollback-detection`

You are the Acceptance Auditor.

Review inputs:
- Diff: `_bmad-output/implementation-artifacts/code-reviews/fn-1-4-scoped-diff-20260412T205008Z.patch`
- Spec: `_bmad-output/implementation-artifacts/stories/fn-1-4-ota-self-check-and-rollback-detection.md`
- Context: `_bmad-output/project-context.md`

Task:
- Review the diff against the spec and context docs.
- Check for:
  - violations of acceptance criteria
  - deviations from spec intent
  - missing implementation of specified behavior
  - contradictions between spec constraints and actual code
  - missing or misaligned tests for promised behavior

Output format:
- Return findings only.
- Use a Markdown list.
- Each finding must include:
  - short title
  - violated AC or constraint
  - severity: `critical`, `high`, `medium`, or `low`
  - evidence with file and line reference
  - concise explanation
- If there are no findings, say `No findings.`

