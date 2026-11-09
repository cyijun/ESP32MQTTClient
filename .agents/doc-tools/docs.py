#!/usr/bin/env python3
"""Portable project documentation checks. Python 3.10+, standard library only."""
import argparse
from datetime import date
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

SKILLS='.agents/skills'
LOCAL='.agents/doc-tools/.local'
MANIFEST='.agents/doc-tools/archive-seals.json'
PARTS={
 'docs':['doc-maintenance/SKILL.md','doc-maintenance/references/standard.md','doc-maintenance/references/structure-hierarchy.md','doc-maintenance/references/review.md'],
 'prose':['prose-standard/SKILL.md','prose-standard/references/examples.md'],
 'trim':['trim-session-residue/SKILL.md','trim-session-residue/references/examples.md','trim-session-residue/references/recall-batteries.md'],
 'archive':['archive-agent-notes/SKILL.md','doc-maintenance/references/note-policy.md'],
}
CLASSES={'feature','bug-fix','simplification','architecture','process','testing'}

def path(root, rel):
    p=Path(rel)
    if p.is_absolute() or '..' in p.parts or not p.parts:
        raise ValueError('Not a project-relative path: '+str(rel))
    target=root/p
    if not target.resolve().is_relative_to(root):
        raise ValueError('Path escapes project: '+str(rel))
    return target

def load(root):
    config=json.loads(path(root,'.agents/docs-config.json').read_text())
    if config.get('version')!=1 or config.get('notes_format') not in ('native','managed'):
        raise ValueError('Unsupported configuration version or decision format')
    for key in ('docs_root','notes_root','archive_root'):
        path(root,config[key])
    return config

def save(p,data):
    p.parent.mkdir(parents=True,exist_ok=True)
    # No follow-through writes to a symlinked state file.
    if p.is_symlink():raise ValueError('Refusing to write through symlink: '+str(p))
    temp=p.with_name(p.name+'.tmp')
    if temp.exists() or temp.is_symlink():raise ValueError('Temporary path already exists: '+str(temp))
    try:
        temp.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n')
        temp.replace(p)
    finally:
        if temp.exists():temp.unlink()

def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def prose(text):
    out=[];fence=None
    for line in text.splitlines():
        m=re.match(r'^\s*(`{3,}|~{3,})',line)
        if m:
            token=m.group(1)
            if fence is None:fence=token
            elif token[0]==fence[0] and len(token)>=len(fence):fence=None
            continue
        if fence is None:out.append(line)
    return '\n'.join(out)

def excluded(rel,config):
    prefixes=config.get('exclude',[])+[config['archive_root']]
    return any(rel==p.rstrip('/') or rel.startswith(p.rstrip('/')+'/') for p in prefixes)

def documents(root,config):
    # Limit checks to project-owned documentation and skills; do not crawl dependencies.
    candidates=set()
    bases=[config['docs_root'],config['notes_root'],SKILLS]
    for base in bases:
        top=path(root,base)
        if not top.exists():continue
        for directory,dirs,files in os.walk(top,followlinks=False):
            dirs[:]=[d for d in dirs if not (Path(directory)/d).is_symlink() and not excluded((Path(directory)/d).relative_to(root).as_posix(),config)]
            for filename in files:
                p=Path(directory)/filename
                rel=p.relative_to(root).as_posix()
                if p.suffix=='.md' and not p.is_symlink() and not excluded(rel,config):candidates.add(rel)
    for name in ('AGENTS.md','README.md','README.zh.md','CONTRIBUTING.md'):
        if path(root,name).is_file():candidates.add(name)
    for rel in config.get('extra_docs',[]):
        if path(root,rel).is_file():candidates.add(rel)
    return sorted(candidates)

def link_errors(root,rel,text):
    errors=[]
    text=prose(text)
    # Mask inline code, including quoted examples of Markdown syntax.
    text=re.sub(r'`+[^`\n]*`+','',text)
    definitions={m.group(1).strip().casefold():m.group(2) for m in re.finditer(r'^\s*\[([^\]]+)\]:\s*(\S+)',text,re.M)}
    links=[m.group(1) for m in re.finditer(r'\]\(\s*(<[^>]+>|[^\s)]+)(?:\s+[^)]*)?\)',text)]
    for m in re.finditer(r'\[([^\]\n]+)\]\[([^\]\n]*)\]',text):
        key=(m.group(2) or m.group(1)).casefold()
        if key not in definitions:errors.append(rel+': undefined reference link '+key)
        else:links.append(definitions[key])
    for target in links:
        target=target.strip('<>')
        if not target or target.startswith('#') or re.match(r'^[a-zA-Z][\w+.-]*:',target):continue
        target=target.split('#')[0].split('?')[0]
        from urllib.parse import unquote
        target=unquote(target)
        # Root-relative website routes are renderer-owned, not local filesystem links.
        if target.startswith('/'):
            if target.startswith(('/Users/','/home/','/tmp/')):errors.append(rel+': machine-specific absolute link '+target)
            continue
        if not (path(root,rel).parent/target).exists():errors.append(rel+': missing link target '+target)
    return errors

def note_errors(root,config):
    if config['notes_format']=='native':return []
    errors=[];base=path(root,config['notes_root'])
    required={'proposed':['Problem','Proposal','Alternatives considered','Acceptance criteria','Risks'],
              'implemented':['Problem','Decision','Alternatives considered','Consequences'],
              'rejected':['Problem','Proposal','Alternatives considered']}
    if (base/'INDEX.md').exists():errors.append('Managed decisions: centralized INDEX.md duplicates path/status metadata')
    for p in base.glob('*'):
        if p.is_dir() and p.name not in {*required,'archived'}:errors.append('Unknown decision lifecycle: '+p.name)
    for lifecycle,sections in required.items():
        for p in (base/lifecycle).rglob('*.md'):
            if p.name in ('AGENTS.md','CLAUDE.md'):continue
            rel=p.relative_to(base/lifecycle)
            if len(rel.parts)!=2 or rel.parts[0] not in CLASSES or not re.match(r'^\d{4}-\d{2}-\d{2}-.+\.md$',p.name):
                errors.append(str(p.relative_to(root))+': expected class/yyyy-mm-dd-topic.md');continue
            try:date.fromisoformat(p.name[:10])
            except ValueError:errors.append(str(rel)+': invalid proposal date')
            lines=p.read_text().splitlines();body=prose(p.read_text())
            status=lines[2] if len(lines)>2 else ''
            expected='Status: '+lifecycle
            if not lines or not re.match(r'^# Agent Note: \S',lines[0]) or len(lines)<4 or lines[1] or lines[3]:errors.append(str(rel)+': invalid header')
            if (lifecycle=='rejected' and not re.match(r'^Status: rejected — \S',status)) or (lifecycle!='rejected' and status!=expected):errors.append(str(rel)+': status disagrees with lifecycle')
            headings=re.findall(r'^## (.+)$',body,re.M)
            if not headings or headings[0]!='Problem':errors.append(str(rel)+': first section must be Problem')
            for section in sections:
                if section not in headings:errors.append(str(rel)+': missing '+section)
            if lifecycle=='implemented' and any(h in ('Proposal','Plan','Migration plan','Acceptance criteria') for h in headings):errors.append(str(rel)+': implemented record still contains proposal sections')
    return errors

def seals(root,config,baseline=None,allow_new=False):
    manifest_path=path(root,MANIFEST)
    manifest=json.loads(manifest_path.read_text()) if manifest_path.exists() else {'version':1,'files':{}}
    if manifest.get('version')!=1 or not isinstance(manifest.get('files'),dict):raise ValueError('Invalid archive manifest')
    errors=[];actual={}
    archive=path(root,config['archive_root'])
    for p in archive.rglob('*') if archive.exists() else []:
        if p.is_file() and p.name not in ('AGENTS.md','CLAUDE.md'):
            rel=p.relative_to(root).as_posix()
            if p.is_symlink():errors.append(rel+': archive symlinks cannot be sealed');continue
            actual[rel]=digest(p)
            if config['notes_format']=='managed' and p.suffix=='.md':
                lines=p.read_text().splitlines()
                if len(lines)<4 or lines[2]!='Status: implemented' or not lines[3].startswith('Archived: '):errors.append(rel+': missing implemented/archive metadata')
                else:
                    try:date.fromisoformat(lines[3][10:])
                    except ValueError:errors.append(rel+': invalid archive date')
                parts=p.relative_to(archive).parts
                if len(parts)!=2 or parts[0] not in CLASSES:errors.append(rel+': invalid archive class path')
    for rel,value in manifest['files'].items():
        path(root,rel)
        if actual.get(rel)!=value:errors.append(rel+': sealed artifact changed or missing')
    for rel in actual:
        if rel not in manifest['files'] and not allow_new:errors.append(rel+': unsealed archive artifact; classify and seal explicitly')
    if baseline:
        result=subprocess.run(['git','-C',str(root),'rev-parse','--verify',baseline+'^{commit}'],capture_output=True,text=True)
        if result.returncode:raise ValueError('Trusted baseline is not a local commit')
        sha=result.stdout.strip()
        listing=subprocess.run(['git','-C',str(root),'ls-tree','-r','--name-only',sha,'--',MANIFEST],capture_output=True,text=True,check=True)
        if listing.stdout.strip():
            old=json.loads(subprocess.run(['git','-C',str(root),'show',sha+':'+MANIFEST],capture_output=True,text=True,check=True).stdout)
            for rel,value in old['files'].items():
                if manifest['files'].get(rel)!=value:errors.append(rel+': frozen manifest entry differs from trusted baseline')
    return errors,actual

def check(root,config,baseline=None):
    errors=[]
    for rel in documents(root,config):
        text=path(root,rel).read_text()
        errors.extend(link_errors(root,rel,text))
        if re.search(r'\{\{[^}]+\}\}|<PROJECT_NAME>|\[INSERT [^\]]+\]',prose(text)):errors.append(rel+': unfinished scaffold placeholder')
    errors.extend(note_errors(root,config))
    for rel,ceiling in config.get('budgets',{}).items():
        p=path(root,rel)
        if not p.is_file():errors.append(rel+': budgeted document missing');continue
        count=len(re.findall(r'[\u3400-\u9fff\uf900-\ufaff]|[^\s\u3400-\u9fff\uf900-\ufaff]+',p.read_text()))
        if not isinstance(ceiling,int) or ceiling<1 or count>ceiling:errors.append(f'{rel}: {count} units exceeds/invalid budget {ceiling}')
    errors.extend(seals(root,config,baseline)[0])
    return errors

def activation_files(root,config,part):
    files=[SKILLS+'/'+s for s in PARTS[part]]
    if part=='docs':
        files+=['AGENTS.md',config['docs_root']+'/AGENTS.md']
    if part=='archive':
        files += [p for p in [config['notes_root']+'/README.md',config['notes_root']+'/AGENTS.md',config['archive_root']+'/AGENTS.md'] if path(root,p).is_file()]
    return files

def activate(root,config,part):
    files=activation_files(root,config,part)
    payload='\n\n'.join('FILE: '+p+'\n'+path(root,p).read_text() for p in files)
    print(payload)
    sys.stdout.flush()
    receipt=path(root,LOCAL+'/activation.json')
    record=json.loads(receipt.read_text()) if receipt.exists() else {'parts':{}}
    record['parts'][part]={p:digest(path(root,p)) for p in files}
    save(receipt,record)
    print('\nDELIVERY RECORDED. Read all content above. If output was truncated, open omitted files directly. Delivery hashes do not prove comprehension or quality.')

def finish(root,config,report_path):
    receipt=path(root,LOCAL+'/activation.json')
    errors=[]
    record=json.loads(receipt.read_text()) if receipt.exists() else {'parts':{}}
    for part in PARTS:
        hashes=record.get('parts',{}).get(part,{})
        for rel in activation_files(root,config,part):
            if hashes.get(rel)!=digest(path(root,rel)):errors.append('Missing/stale same-turn instruction delivery: '+rel)
    report=json.loads(report_path.read_text())
    outputs=report.get('outputs',[])
    if not outputs:errors.append('Initialization needs project-specific outputs, not just copied policies')
    allowed={p for group in PARTS.values() for p in group}
    for item in outputs:
        rel=item.get('path','');p=path(root,rel)
        if rel.startswith(SKILLS+'/') or rel.startswith('.agents/doc-tools/') or p.name=='AGENTS.md':errors.append(rel+': copied policy is not project-specific content')
        if not p.is_file():errors.append(rel+': output missing');continue
        if not item.get('sources'):errors.append(rel+': source evidence missing')
        for source in item.get('sources',[]):
            if not path(root,source).is_file():errors.append(rel+': missing evidence '+source)
        if len(prose(p.read_text()).strip())<80:errors.append(rel+': output too small for an initialization evidence review')
    reviews=report.get('reviews',[])
    needed={'fact-ownership','complete-propositions','session-independent','existing-content-preserved'}
    if not needed.issubset({r.get('principle') for r in reviews}):errors.append('Missing required semantic review categories')
    output_paths={i.get('path') for i in outputs}
    for review in reviews:
        if review.get('path') not in output_paths or len(review.get('finding','').strip())<30:errors.append('Review must name a materialized output and a specific applied finding')
    if not isinstance(report.get('verification'),list) or not report['verification']:errors.append('State operations run or explicitly unverified; no empty verification claims')
    if config['notes_format']=='native' and not report.get('native_decision_review'):errors.append('Existing ADR convention requires a semantic review statement')
    errors.extend(check(root,config))
    if errors:return errors
    save(path(root,LOCAL+'/completion.json'),{'status':'review-recorded','report':report,'outputs_sha256':{r:digest(path(root,r)) for r in output_paths},'limits':'Delivery and review evidence recorded; semantic accuracy remains the author/reviewer responsibility.'})
    return []

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command',choices=['check','activate','seal','finish'])
    parser.add_argument('--root',default='.')
    parser.add_argument('--part',choices=list(PARTS))
    parser.add_argument('--baseline',help='Trusted local Git revision for immutable manifest comparison')
    parser.add_argument('--report',help='Author-written initialization review JSON')
    args=parser.parse_args();root=Path(args.root).resolve();config=load(root)
    if args.command=='activate':
        if not args.part:raise ValueError('activate requires --part')
        activate(root,config,args.part);return
    if args.command=='seal':
        errors,actual=seals(root,config,args.baseline,allow_new=True)
        if not errors:save(path(root,MANIFEST),{'version':1,'files':dict(sorted(actual.items()))})
    elif args.command=='finish':
        if not args.report:raise ValueError('finish requires --report')
        errors=finish(root,config,Path(args.report))
    else:errors=check(root,config,args.baseline)
    if errors:
        print('\n'.join(errors),file=sys.stderr);raise SystemExit(1)
    print(args.command+': passed structural/evidence checks. Semantic accuracy and actual instruction use require review.')
    if config['notes_format']=='native':print('Existing decision format preserved; native decision grammar is not mechanically checked.')
    if not args.baseline:print('Archive hashes checked locally; trusted-baseline comparison was not requested.')

if __name__=='__main__':
    try:main()
    except (ValueError,OSError,KeyError,TypeError,subprocess.CalledProcessError) as exc:raise SystemExit('docs: '+str(exc))
