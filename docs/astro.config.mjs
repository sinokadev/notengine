// @ts-check
import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';
import starlightVersions from 'starlight-versions';

// https://astro.build/config
export default defineConfig({
	integrations: [
		starlight({
			title: 'Not Engine Docs',
			social: [{ icon: 'github', label: 'GitHub', href: 'https://github.com/sinokadev/notengine' }],
			sidebar: [
				{
					label: '가이드',
					items: [
						{ label: '시작하기', slug: 'guides/getting-started' },
					],
				},
				{
					label: '문서',
					items: [{ autogenerate: { directory: 'docs' } }],
				},
			],
			plugins: [
				starlightVersions({
					current: {
                        label: 'v1.5a',
                    },
					versions: [
						{ slug: 'v0.0', label: 'v0.0' },
					],
				}),
			],
		}),
	],
});
