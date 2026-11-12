import { NextRequest, NextResponse } from "next/server";

const CANONICAL_HOST = "phostide.doticca.com";

/** 308 vercel.app (and www) onto the public custom domain so search indexes one origin. */
export function proxy(request: NextRequest) {
  const host = (request.headers.get("host") ?? "").split(":")[0].toLowerCase();

  if (host === "phostide.vercel.app" || host === `www.${CANONICAL_HOST}`) {
    const dest = request.nextUrl.clone();
    dest.protocol = "https:";
    dest.hostname = CANONICAL_HOST;
    dest.port = "";
    return NextResponse.redirect(dest, 308);
  }

  return NextResponse.next();
}

export const config = {
  matcher: [
    "/((?!_next/static|_next/image|.*\\.(?:png|jpg|jpeg|gif|webp|svg|ico|webm|woff2?)$).*)",
  ],
};
