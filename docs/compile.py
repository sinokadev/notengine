from pathlib import Path
from dataclasses import dataclass, field
from wenmode import Wenmode
from wenmode.plugins import frontmatter
import shutil
import time

@dataclass
class Content:
    order: int
    name: str
    title: str
    description: str
    content: str  # HTML 변환 결과

@dataclass
class Chapter:
    order: int
    name: str
    contents: list[Content] = field(default_factory=list)

@dataclass
class Page:
    name: str
    index: str
    page: str
    chapters: list[Chapter] = field(default_factory=list)

start_time = time.time()

wen = Wenmode(plugins=[frontmatter])

templates_path = Path("./template/page")
contents_path = Path("./content")

# 공용 템플릿 읽기
index_file = templates_path / "index.html"
page_file = templates_path / "page.html"

index_content = index_file.read_text(encoding="utf-8") if index_file.is_file() else None
page_content = page_file.read_text(encoding="utf-8") if page_file.is_file() else None

assert (index_content is not None) and (page_content is not None), (
    f"{templates_path.name}: index.html 또는 page.html이 없습니다."
)


def parse_content_file(file_path: Path) -> Content:
    assert file_path.is_file(), f"{file_path.parent.name}: 마크다운 파일이 아닙니다."

    content_markdown = file_path.read_text(encoding="utf-8")
    content_html = wen.render(content_markdown)

    parsed_ast = wen.parse(content_markdown)
    content_frontmatter = parsed_ast.data.get("frontmatter") or {}

    assert ("title" in content_frontmatter) and ("description" in content_frontmatter), (
        f"{file_path.parent.name}/{file_path.name}: Front Matter에 'title'과 'description' 항목은 필수입니다."
    )

    file_stem = file_path.stem
    has_prefix = len(file_stem) > 2 and file_stem[0].isdigit() and file_stem[1] in ("-", "_")
    
    order = int(file_stem[0]) if file_stem[0].isdigit() else 0
    name = file_stem[2:] if has_prefix else file_stem

    return Content(
        order=order,
        name=name,
        title=content_frontmatter["title"],
        description=content_frontmatter["description"],
        content=content_html
    )


# content/ 아래의 각 디렉터리를 독립된 Page로 파싱
pages_data: list[Page] = []

for target_page_dir in (item for item in contents_path.iterdir() if item.is_dir()):
    page_name = target_page_dir.name
    page_chapters: list[Chapter] = []

    root_files: list[Path] = []
    chapter_dirs: list[Path] = []

    for item in target_page_dir.iterdir():
        if item.is_dir():
            chapter_dirs.append(item)
        elif item.is_file() and item.suffix == ".md":
            root_files.append(item)

    # 루트 파일들 (이름 없는 챕터)
    if root_files:
        standalone_contents = [parse_content_file(f) for f in root_files]
        standalone_contents.sort(key=lambda c: c.order)
        page_chapters.append(Chapter(order=0, name="", contents=standalone_contents))

    # 하위 챕터 디렉터리들
    for chapter_dir in chapter_dirs:
        chapter_contents: list[Content] = []
        
        for content_file in chapter_dir.iterdir():
            if content_file.is_file() and content_file.suffix == ".md":
                chapter_contents.append(parse_content_file(content_file))

        chapter_contents.sort(key=lambda c: c.order)

        dir_name = chapter_dir.name
        has_prefix = len(dir_name) > 2 and dir_name[0].isdigit() and dir_name[1] in ("-", "_")
        
        ch_order = int(dir_name[0]) if dir_name[0].isdigit() else 0
        ch_name = dir_name[2:] if has_prefix else dir_name

        page_chapters.append(Chapter(order=ch_order, name=ch_name, contents=chapter_contents))

    page_chapters.sort(key=lambda ch: ch.order)

    pages_data.append(Page(
        name=page_name,
        index=index_content,
        page=page_content,
        chapters=page_chapters
    ))


# 컴파일
dist_path = Path("./dist")

if dist_path.exists():
    shutil.rmtree(dist_path)
dist_path.mkdir(parents=True, exist_ok=True)

root_index_template = Path("./template/index.html")

if root_index_template.exists():
    shutil.copy(root_index_template, dist_path / "index.html")

content_count = 0
chapter_count = 0

for page in pages_data:
    page_dist_dir = dist_path / page.name
    page_dist_dir.mkdir(parents=True, exist_ok=True)

    page_index_html = ""

    for chapter in page.chapters:
        chapter_count += 1
        # 챕터별 출력 경로 결정
        if chapter.name:
            chapter_dist_dir = page_dist_dir / chapter.name
            chapter_dist_dir.mkdir(parents=True, exist_ok=True)
            page_index_html += f"<br><b>{chapter.name}</b><br>"
        else:
            chapter_dist_dir = page_dist_dir

        chapter_index_html = ""

        # 각 마크다운 파일(Content) html 생성
        for content in chapter.contents:
            content_count += 1
            rendered_page = page.page.format(
                title=content.title,
                chapter=chapter.name if chapter.name else page.name,
                content=content.content
            )

            output_file = chapter_dist_dir / f"{content.name}.html"
            output_file.write_text(rendered_page, encoding="utf-8")

            # URL 경로 생성
            url_path = f"/{page.name}/{chapter.name}/{content.name}" if chapter.name else f"/{page.name}/{content.name}"
            
            link_tag = f'<a href="{url_path}">{content.title}</a><br>'
            chapter_index_html += link_tag
            page_index_html += link_tag

        # 챕터 단위 index 생성 (이름있는 챕터만, 없을경우 페이지 단위 index만 존재)
        if chapter.name:
            rendered_chapter_index = page.index.format(name=chapter.name,content=chapter_index_html)
            chapter_index_file = chapter_dist_dir / "index.html"
            chapter_index_file.write_text(rendered_chapter_index, encoding="utf-8")

    # 페이지 단위 index 생성
    rendered_page_index = page.index.format(name=page.name,content=page_index_html)
    page_index_file = page_dist_dir / "index.html"
    page_index_file.write_text(rendered_page_index, encoding="utf-8")

end_time = time.time()
execution_time = end_time - start_time

print("컴파일 완료")
print(f"실행 시간: {execution_time:.4f}초 | 페이지 갯수: {len(pages_data)} | 챕터 갯수: {chapter_count} | 문서 갯수: {content_count}")