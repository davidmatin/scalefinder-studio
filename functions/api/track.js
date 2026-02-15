export async function onRequestPost(context) {
  const { request, env } = context;

  const origin = request.headers.get("Origin") || "";
  const allowed =
    origin === "https://scalefinder.studio" ||
    origin.startsWith("http://localhost");

  const corsHeaders = {
    "Access-Control-Allow-Origin": allowed ? origin : "https://scalefinder.studio",
    "Access-Control-Allow-Methods": "POST, OPTIONS",
    "Access-Control-Allow-Headers": "Content-Type",
  };

  if (request.method === "OPTIONS") {
    return new Response(null, { headers: corsHeaders });
  }

  try {
    const body = await request.json();
    const { event, properties } = body;

    if (!event || typeof event !== "string" || event.length > 100) {
      return new Response(JSON.stringify({ error: "Invalid event" }), {
        status: 400,
        headers: { ...corsHeaders, "Content-Type": "application/json" },
      });
    }

    const sessionId = properties?.session_id || "unknown";
    const userId = properties?.user_id || null;
    const userAgent = request.headers.get("User-Agent") || "";
    const country = request.headers.get("CF-IPCountry") || "unknown";

    // Remove session_id and user_id from properties to avoid duplication
    const { session_id: _s, user_id: _u, ...eventProps } = properties || {};

    await env.DB.prepare(
      `INSERT INTO analytics_events
       (event_name, session_id, user_id, properties, user_agent, country)
       VALUES (?, ?, ?, ?, ?, ?)`
    )
      .bind(event, sessionId, userId, JSON.stringify(eventProps), userAgent, country)
      .run();

    return new Response(JSON.stringify({ ok: true }), {
      status: 200,
      headers: { ...corsHeaders, "Content-Type": "application/json" },
    });
  } catch (err) {
    console.error("Analytics error:", err);
    return new Response(null, { status: 204, headers: corsHeaders });
  }
}

export async function onRequestOptions(context) {
  const origin = context.request.headers.get("Origin") || "";
  const allowed =
    origin === "https://scalefinder.studio" ||
    origin.startsWith("http://localhost");

  return new Response(null, {
    headers: {
      "Access-Control-Allow-Origin": allowed ? origin : "https://scalefinder.studio",
      "Access-Control-Allow-Methods": "POST, OPTIONS",
      "Access-Control-Allow-Headers": "Content-Type",
    },
  });
}
